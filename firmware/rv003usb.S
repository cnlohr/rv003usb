// Test

#include "ch32v003fun.h"
#include "rv003usb.h"

#define CFGLR_OFFSET 0
#define INDR_OFFSET 8
#define BSHR_OFFSET 16

#define SYSTICK_CNT 0xE000F008

// This is 6 * n + 3 cylces
#define nx6p3delay( n ) li s0, ((n)+1); 1: c.addi s0, -1; c.bnez s0, 1b

#define DEBUG_TICK_SETUP la x4, (TIM1_BASE + 0x24) 	// for debug
#define DEBUG_TICK_MARK sw x0, 0(x4)

.global test_memory
.global rv003usb_internal_data
.global rv003usb_handle_packet
.global usb_send_data

/* Register map
	zero, ra, sp, gp, tp, t0, t1, t2
Compressed:
	s0, s1,	a0, a1, a2, a3, a4, a5
*/

.global EXTI7_0_IRQHandler
.balign 4
EXTI7_0_IRQHandler:
	addi	sp,sp,-56
	sw	a0, 0(sp)
	sw	a5, 20(sp)
	sw	a1, 4(sp)

	la a5, USB_GPIO_BASE
	lw a0, INDR_OFFSET(a5)
	c.andi a0, USB_DMASK
	c.beqz a0, handle_se0_keepalive

	// If we've gotten here, it's a data packet.
	lw a1, INDR_OFFSET(a5);c.andi a1, USB_DMASK;c.xor a1, a0;c.bnez a1, syncdone
	lw a1, INDR_OFFSET(a5);c.andi a1, USB_DMASK;c.xor a1, a0;c.bnez a1, syncdone
	lw a1, INDR_OFFSET(a5);c.andi a1, USB_DMASK;c.xor a1, a0;c.bnez a1, syncdone
	lw a1, INDR_OFFSET(a5);c.andi a1, USB_DMASK;c.xor a1, a0;c.bnez a1, syncdone
	lw a1, INDR_OFFSET(a5);c.andi a1, USB_DMASK;c.xor a1, a0;c.bnez a1, syncdone

	// OPTIMIZATION: I don't think we need this one.
	lw a1, INDR_OFFSET(a5);c.andi a1, USB_DMASK;c.xor a1, a0;c.bnez a1, syncdone

	j interrupt_complete
syncdone:

	// This will be called when we have synchronized our USB.  We can put our
	// preamble detect code here.  But we have a whole free USB bit cycle to
	// do whatever we feel like.

	// NOTE NOTE TODO: This can be abused to shift the time when we are
	// sampling in our phase.
	sw	a2, 8(sp)
	sw	a3, 12(sp)
	sw	a4, 16(sp)
	sw	s0, 24(sp)
	sw	s1, 28(sp)
	sw	t0, 32(sp)
	sw	t1, 36(sp)
	sw  t2, 40(sp) // 16 cycles
	sw	x4, 48(sp)
	sw  ra, 52(sp)

	// r0 points to SYSTICK 
	// a5 points to our port's input.
	la  t0, SYSTICK_CNT
	la  t2, rv003usb_internal_data

	DEBUG_TICK_SETUP

	c.xor a1, a0 // Recover a1

	DEBUG_TICK_MARK

preamble_loop:
	lw a0, INDR_OFFSET(a5);
	c.andi a0, USB_DMASK;
	c.xor a0, a1;
	c.xor a1, a0; // Recover a1.
	c.beqz a0, done_preamble
	// 6 cycles
	nx6p3delay( 3 ); // 21 cycles
	c.nop
	j preamble_loop // 4 cycles
done_preamble:

	// Need to delay about 24 cycles
	li a2, 0

	li a4, 0xa001
	li a3, 0xf0eb // so that if the CRC is correct, it will be 0.

	// This is the first bit that matters.
	li s0, 7 // 1 runs.

	c.nop; c.nop; c.nop;
	c.nop; c.nop; c.nop; c.nop;
	//XXX TODO: Tune?

	/* register meanings:
		* T0 = temp
		* T1 = TEMPORARY

		* A0 = temp / current bit value.
		* A1 = last-frame's GPIO values.
		* A2 = The running word 
		* A3 = Running CRC
		* A4 = Polynomial
		* A5 = GPIO Offset

		* S0 = Bit Stuff Place
		* S1 = # output bits remaining.
*/
	// XXX TODO: Do one byte here to determine the header byte and from that set the CRC.
	li s1, 8
	li a2, 0

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

packet_type_loop:
	nx6p3delay(1);
	c.nop
	DEBUG_TICK_MARK

	c.lw a0, INDR_OFFSET(a5);
	c.andi a0, USB_DMASK;
	c.beqz a0, done_usb_message // Not se0 complete, that can't happen here and be valid.
	c.xor a0, a1;
	c.xor a1, a0; // Recover a1, for next cycle
	// a0 = 00 for 1 and 11 for 0

	// No CRC for the header.
	c.srli a0, USB_DM
	c.addi a0, 1 // 00 -> 1, 11 -> 100
	c.andi a0, 1 // If 1, 1 if 0, 0.

	// Write header into byte in reverse order, because we can.
	c.slli a2, 1
	c.or a2, a0

	// Handle bit stuffing rules.
	c.addi a0, -1 // 0->0xffffffff 1->0
	c.or s0, a0
	c.addi s0, -1
	c.andi s0, 7

	c.addi s1, -1
	c.bnez s1, packet_type_loop

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////	

	// XXX Here, figure out CRC polynomial.

	li s1, (USB_BUFFER_SIZE*8) // # of bits

	// WARNING: a0 is bit-wise backwards here.
	// 0xb4 for instance is a setup packet.
	//
	// When we get here, packet type is loaded in A2.
	// If packet type is 0xXX01 or 0xXX11
	// the LSBs are the inverted packet type.
	// we can branch off of bit 2.
	andi a0, a2, 2

	// if a0 is 1 then it's DATA (full CRC) otheriwse,
	// (0) for setup or PARTIAL CRC.
	// a4 is preloaded with 0xa001 up above, SO we only
	// need to do something IF it's 0!
	c.beqz a0, data_crc // XXX TODO REV'd (I guess it's backwards?)
	li a4, 0x14	
	li a3, 0x1e
data_crc:
	DEBUG_TICK_MARK


#define HANDLE_EOB_YES \
	sb a2, 0(t2); /* Save the byte off. TODO: Is unaligned byte access to RAM slow? */ \
	.word 0x00138393; /*addi t2, t2, 1;*/

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////	

is_end_of_byte:
	HANDLE_EOB_YES

	// end-of-byte.
bit_process:
	// Debug blip
	DEBUG_TICK_MARK
	c.lw a0, INDR_OFFSET(a5);
	c.andi a0, USB_DMASK;
	c.beqz a0, se0_complete
	c.xor a0, a1;
	c.xor a1, a0; // Recover a1, for next cycle
/*
#define HANDLE_CRC \
	c.xor a0, a3; \
	c.andi a0, 1; \
	c.addi a0, -1; \
	and a0, a0, t0; \
	c.srli a3, 1; \
	c.xor a3, a0
*/
#define HANDLE_NEXT_BYTE(is_end_of_byte)  \
	c.addi s1, -1; \
	andi a0, s1, 7; /* s1 could be really really big */ \
	c.beqz a0, is_end_of_byte /* 4 cycles for this section. (Checked) (Sometimes 5)? */ 

	c.bnez a0, handle_zero_bit

handle_one_bit:
	c.addi s0, -1; // Count # of runs of 1 (subtract 1)
	c.srli a2, 1;  // shift a2 down by 1
	//c.li a0, 0

	//HANDLE_CRC (1 bit)
	andi a0, a3, 1
	c.addi a0, -1
	c.and a0, a4
	c.srli a3, 1
	c.xor a3, a0

	c.li a0, 1
	c.slli a0, 7
	or a2,a2, a0;   // It's a 1, or it in.
	c.beqz s0, handle_bit_stuff;
	HANDLE_NEXT_BYTE(is_end_of_byte)

	c.nop
	c.nop
	c.nop
	c.bnez s1, bit_process // + 4 cycles
	c.j done_usb_message

handle_zero_bit:
	c.li s0, 7      // reset runs-of-one.
	c.srli a2, 1;   // shift a2 down by 1

	// Handle CRC (0 bit)
	andi a0, a3, 1
	neg a0, a0
	c.and a0, a4
	c.srli a3, 1
	c.xor a3, a0

	//c.li a0, 1
	//HANDLE_CRC
	HANDLE_NEXT_BYTE(is_end_of_byte)

	c.nop
	c.nop
	c.bnez s1, bit_process // + 4 cycles
	c.j done_usb_message

handle_bit_stuff:
	// We want to wait a little bit, then read another byte, and make
	// sure everything is well, before heading back into the main loop
	// Debug blip
	HANDLE_NEXT_BYTE(is_end_of_byte_and_bit_stuffed)
	c.j is_not_end_of_byte
is_end_of_byte_and_bit_stuffed:
	HANDLE_EOB_YES

is_not_end_of_byte:

	DEBUG_TICK_MARK
	DEBUG_TICK_MARK

	c.lw a0, INDR_OFFSET(a5);
	c.andi a0, USB_DMASK;
	c.beqz a0, se0_complete
	c.xor a0, a1;
	c.xor a1, a0; // Recover a1, for next cycle.

	slti a0, a0, 1<<USB_DM // A0 = 0 if 0 bit, A0 = 1 if 1 bit.

	// If a0 is a 1 then that's bad.
	c.bnez a0, done_usb_message

	c.andi s0, 7  // if 0 set it to 7, if 
	c.addi s0, -1 // Count # of runs of 1 (subtract 1)

	// debug blip chain.
	DEBUG_TICK_MARK
	DEBUG_TICK_MARK
	DEBUG_TICK_MARK
	DEBUG_TICK_MARK
	DEBUG_TICK_MARK
	DEBUG_TICK_MARK
	DEBUG_TICK_MARK
	DEBUG_TICK_MARK

	c.bnez s1, bit_process // + 4 cycles

se0_complete:
	DEBUG_TICK_MARK
	DEBUG_TICK_MARK
	DEBUG_TICK_MARK

	// Special: handle ACKs?
	// Now we have to decide what we're doing based on the
	// packet type.
	la  a1, rv003usb_internal_data
	lw  a0, 0(a1)
	sub a4, a1, t2      //A4 = # of bytes read..
	c.mv s0, a0
	c.srli s0, 4
	c.andi s0, 0xf
	// If bad CRC, skip ahead.
//	c.bnez a3, done_usb_message

	// Good CRC!
	lw	x4, 48(sp) // restore x4 for whatever in C land.

	// a0 contains first 4 bytes.
	la ra, done_usb_message_in

	c.addi s0, -0b0100 // ACK packet (but backwards)
	c.beqz s0, usb_pid_handle_ack

	andi a5, a0, 4
	c.beqz a5, is_data_packet
		// Else: Is a setup packet
		addi s0,s0, 0b0100-0b1011 // a SETUP packet
		c.beqz s0, usb_pid_handle_setup
		c.addi s0, (0b1011-0b1000) //a OUT token
		c.beqz s0, usb_pid_handle_out
		c.addi s0, (0b1000-0b1001) //an IN token
		c.beqz s0, usb_pid_handle_in
		c.j done_usb_message_in
	is_data_packet:
		// Data packet. Handle.
		c.addi s0, 0b0100-0b1100 // a DATA0 packet
		c.li   a2, 0
		c.beqz s0, usb_pid_handle_data
		c.addi s0, (0b1100-0b1101) // a DATA1 packet
		c.li   a2, 1
		c.beqz s0, usb_pid_handle_data

done_usb_message:
done_usb_message_in:

	lw	a2, 8(sp)
	lw	a3, 12(sp)
	lw	a4, 16(sp)
	lw	s0, 24(sp)
	lw	s1, 28(sp)
	lw	t0, 32(sp)
	lw	t1, 36(sp)
	lw	t2, 40(sp)
	lw	x4, 48(sp)
	lw  ra, 52(sp)

interrupt_complete:
	// Acknowledge interrupt.
	// EXTI->INTFR = 1<<4
	la a5, EXTI_BASE + 20
	li a0, (1<<USB_DP)
	sw a0, 0(a5)

	// Restore stack.
	lw	a0, 0(sp)
	lw	a5, 20(sp)
	lw	a1, 4(sp)
	addi	sp,sp,56
	mret

handle_se0_keepalive:
	// In here, we want to do smart stuff with the
	// 1ms tick.
	j interrupt_complete












//void usb_send_data( uint8_t * data, uint32_t length, uint32_t poly_function );
.balign 4
usb_send_data:
	addi	sp,sp,-12
	sw	s0, 0(sp)
	sw	s1, 4(sp)
	sw  x4, 8(sp)
	//sw  s2, 8(sp) // backup if we want the CRC or not.

	// t0 is our polynomial
	// a2 is our running CRC.

	la x4, (TIM1_BASE + 0x24) 	// for debug

	// XXX TODO: This logic is so bad.
	li t0, 0xa001
	c.beqz a2, data_polynomial
	li t0, 0x00
	li a2, 0x00
	c.j done_data
//	li t0, 0x14
//	li a2, 0x1f
data_polynomial:

	lb a3, 1(a0)
	c.andi a3, 0xf
	// Check to see if it's data0 or data1.
	c.addi a3, -0b0011
	c.beqz a3, is_data_0

	// If data1
	li a2, 0x5ed7
	c.j done_data
	c.nop
is_data_0:
	li a2, 0xd6d7

	andi a3, a0, 1
	beqz a3, done_data
	DEBUG_TICK_MARK
	DEBUG_TICK_MARK

done_data:

	// t0 is used for CRC
	// t1 is free
	// t2 is totally free.

	// s1 is our last "state"
	//    bit 0 is last "physical" state,
	//    
	// s0 is our current "bit" / byte / temp.

	// a0 is our data
	// a1 is is our length
	// a2 our CRC
	// a3 is TEMPORARY
	// a4 is used for bit stuffing.
	// a5 is the output address.

	//XXX TODO Forgot about bit stuffing.

	c.slli a1, 3 // bump up one extra to be # of bits

	la a5, USB_GPIO_BASE
	li s1, (1<<USB_DP) | (1<<(USB_DM+16))
	li t1, (1<<USB_DM) | (1<<(USB_DP+16)) | (1<<USB_DP) | (1<<(USB_DM+16));
	c.sw s1, BSHR_OFFSET(a5)
	
	c.lw s0, CFGLR_OFFSET(a5)

	li a3, ~((0b1111<<(USB_DM*4)) | (0b1111<<(USB_DP*4)))
	and s0, a3, s0


	// Convert D+/D- into 2MHz outputs
	li a3, ((0b0010<<(USB_DM*4)) | (0b0010<<(USB_DP*4)))
	or s0, a3, s0

	//00: Universal push-pull output mode
	c.sw s0, CFGLR_OFFSET(a5)

	//xor s1, s1, t1
	//c.sw s1, BSHR_OFFSET(a5)

	c.nop;	c.nop;	c.nop;	c.nop
	c.nop;	c.nop;	c.nop;	c.nop

	// This creates a preamble, which is alternating 1's and 0's
	// and then it sets the same state.
//	li s0, 0b10000000

//	c.j send_inner_loop
load_next_word:
	lh s0, 0(a0) //replace me with the QingKe v2 "special" opcode for this.
	//c.addi a0, 2
	.long 0x00250513 // addi a0, a0, 2
send_inner_loop:
	/* High-level:
		* extract the LSB from s0
		* If it's 0, we FLIP the USB pin state.
		* If it's 1, we don't flip.
		* Regardless of the state, we still compute CRC.
		* We have to decrement our bit stuffing index.
		* If it is 0, we can reset our bit stuffing index.
	*/

	// a3 is now the lsb of s0 (the 'next bit' to read out)
	c.mv a3, s0
	c.srli s0, 1 // Shift down into the next bit.
	c.andi a3, 1
	// If a3 is 0, we should FLIP
	// if a3 is 1, we should NOT flip.

	c.beqz a3, send_zero_bit
//send_one_bit:
	//HANDLE_CRC (1 bit)
	andi a3, a2, 1
	c.addi a3, -1
	and a3, a3, t0
	c.srli a2, 1
	c.xor a2, a3

	c.nop
	c.addi a4, -1
	c.beqz a4, insert_stuffed_bit
	c.nop
	c.j cont_after_jump
//Send 0 bit. (Flip)
send_zero_bit:

	// Handle CRC (0 bit)
	// a2 is our running CRC
	// a3 is temp
	// t0 is polynomial.

	// XXX WARNING: this was by https://github.com/cnlohr/rv003usb/issues/7 
	// TODO Check me!
	slli a3,a2,31 // Put a3s LSB into a0s MSB
	c.srai a3,31    // Copy MSB into all other bits
	c.srli a2,1
	and a3,a3,t0
	c.xor  a2,a3

	// Flip s1 (our bshr setting) by xoring it.
	// 10.....01
	// 11.....11 (xor with)
	// 01.....10
	xor s1, s1, t1
	sw s1, BSHR_OFFSET(a5)

	c.li a4, 6 // reset bit stuffing.
	// DO NOT flip.  Allow a4 to increment.
cont_after_jump:

send_end_bit_complete:
	c.addi a1, -1
	c.beqz a1, done_sending_data
	andi a3, a1, 15
	c.beqz a3, load_next_word
	// Wait an extra 4-5 cycles.
	c.nop
	c.nop
	c.nop
	c.j send_inner_loop

done_sending_data:
	// BUT WAIT!! MAYBE WE NEED TO CRC!
	beqz t0, no_really_done_sending_data
	not s0, a2 // get read to send out the CRC.
	li t0, 0 // reset poly - we don't want it anymore.
	li a1, 16 // Load 16 more bits out
	c.j send_inner_loop

no_really_done_sending_data:

//	c.bnez a2, poly_function  TODO: Uncomment me!

	c.nop; c.nop; c.nop; c.nop; c.nop; c.nop; c.nop; c.nop;

	// Need to perform an SE0.
	li s1, (1<<(USB_DP+16)) | (1<<(USB_DM+16))
	c.sw s1, BSHR_OFFSET(a5)

	c.nop; c.nop; c.nop; c.nop; c.nop; c.nop; c.nop; c.nop;
	c.nop; c.nop; c.nop; c.nop; c.nop; c.nop; c.nop; c.nop;
	c.nop; c.nop; c.nop; c.nop; c.nop; c.nop; c.nop; c.nop;
	c.nop; c.nop; c.nop; c.nop; c.nop; c.nop; c.nop; c.nop;
	c.nop; c.nop; c.nop; c.nop; c.nop; c.nop; c.nop; c.nop;
	c.nop; c.nop; c.nop; c.nop; c.nop; c.nop; c.nop; c.nop;
	li s1, (1<<(USB_DP)) | (1<<(USB_DM+16))
	c.sw s1, BSHR_OFFSET(a5)

	lw s1, CFGLR_OFFSET(a5)
	// Convert D+/D- into inputs.
	li a3, ~((0b11<<(USB_DM*4)) | (0b11<<(USB_DP*4)))
	and s1, a3, s1
	// 01: Floating input mode.
	li a3, ((0b01<<(USB_DM*4+2)) | (0b01<<(USB_DP*4+2)))
	or s1, a3, s1
	sw s1, CFGLR_OFFSET(a5)

	lw	s0, 0(sp)
	lw	s1, 4(sp)
	lw  x4, 8(sp)
	addi	sp,sp,12
	ret

poly_function:
	mv s0, a3
	c.li a1, 5
	c.j send_inner_loop


insert_stuffed_bit:
	c.nop;	c.nop;	c.nop;	c.nop;
	c.nop;	c.nop;	c.nop;	c.nop;
	c.nop;	c.nop;	c.nop;	c.nop;
	c.nop;	c.nop;	c.nop;	c.nop;
	c.nop;
	c.li a4, 7 // reset bit stuffing.
	xor s1, s1, t1
	sw s1, BSHR_OFFSET(a5)
	j send_end_bit_complete
