// Test

#include "ch32v003fun.h"
#include "rv003usb.h"

#define CFGLR_OFFSET 0
#define INDR_OFFSET 8
#define BSHR_OFFSET 16

#define SYSTICK_CNT 0xE000F008

// This is 6 * n + 3 cylces
#define nx6p3delay( n ) li s0, ((n)+1); 1: c.addi s0, -1; c.bnez s0, 1b


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

	la x4, (TIM1_BASE + 0x24) 	// for debug

	c.xor a1, a0 // Recover a1

	sw x0, 0(x4)

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
	c.nop; c.nop; c.nop; c.nop;
	c.nop;  //XXX TODO: Tune?

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
	sw x0, 0(x4)
	sw x0, 0(x4)
	sw x0, 0(x4)
	sw x0, 0(x4)
	sw x0, 0(x4)

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
	c.bnez a0, data_crc
	li a4, 0x14	
	li a3, 0x1e
data_crc:
	sw x0, 0(x4)
	//sw x0, 0(x4)


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
	sw x0, 0(x4)
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
	//HANDLE_CRC
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

	// Handle CRC
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

	sw x0, 0(x4)
	sw x0, 0(x4)

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
	sw x0, 0(x4)
	sw x0, 0(x4)
	sw x0, 0(x4)
	sw x0, 0(x4)
	sw x0, 0(x4)
	sw x0, 0(x4)
	sw x0, 0(x4)
	sw x0, 0(x4)

	c.bnez s1, bit_process // + 4 cycles

se0_complete:
	sw x0, 0(x4)
	sw x0, 0(x4)
	sw x0, 0(x4)

	// Special: handle ACKs?
	// Now we have to decide what we're doing based on the
	// packet type.
	la  a1, rv003usb_internal_data
	lw  a0, 0(a1)
	c.mv a4, a0
	c.srli a4, 4
	c.andi a4, 0xf
	c.addi a4, -0b0100 // ACK packet (but backwards)
	c.beqz a4, usb_pid_handle_ack
	// If bad CRC, skip ahead.
	c.bnez a3, done_usb_message

	// Good CRC!
	lw	x4, 48(sp) // restore x4 for whatever in C land.

	// a0 contains first 4 bytes.
	la ra, done_usb_message_in

	andi a5, a0, 4
	c.beqz a5, is_data_packet
		// Else: Is a setup packet
		addi a4,a4, 0b0100-0b1011 // a SETUP packet
		c.beqz a4, usb_pid_handle_setup
		c.addi a4, (0b1011-0b1000) //a OUT token
		c.beqz a4, usb_pid_handle_out
		c.addi a4, (0b1000-0b1001) //an IN token
		c.beqz a4, usb_pid_handle_in
		c.j done_usb_message_in
	is_data_packet:
		// Data packet. Handle.
		c.addi a4, 0b0100-0b1100 // a DATA0 packet
		c.li   a2, 0
		c.beqz a4, usb_pid_handle_data
		c.addi a4, (0b1100-0b1101) // a DATA1 packet
		c.li   a2, 1
		c.beqz a4, usb_pid_handle_data

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
usb_send_data:
	addi	sp,sp,-8
	sw	s0, 0(sp)
	sw	s1, 4(sp)

	// t0 is our polynomial
	// a2 is our running CRC.

	// t1 is bit run length
	li a2, 0x1f

	li t1, 7
	li t0, 0
	c.beqz a2, no_polynomial
	li t0, 0x14
no_polynomial:

	// s1 is our last "state" (lsb is last "logical" state, bit 1 is last physical state)
	// s0 is our current "bit" / byte / temp.

	// a0 is our data
	// a1 is is our length
	// a2 our polynomial
	// a3 is TEMPORARY
	// a4 is our LINE STATE
	// a5 is the output address.

	//XXX TODO Forgot about bit stuffing.

	c.slli a1, 3 // bump up one extra to be # of bits

	la a5, USB_GPIO_BASE
	li a1, (1<<USB_DM) | (1<<(USB_DP+16));
	c.sw a1, BSHR_OFFSET(a5)
	
	c.lw s1, CFGLR_OFFSET(a5)

	li a3, ~((0b1111<<(USB_DM*4)) | (0b1111<<(USB_DP*4)))
	and s1, a3, s1


	// Convert D+/D- into 2MHz outputs
	li a3, ((0b0010<<(USB_DM*4)) | (0b0010<<(USB_DP*4)))
	or s1, a3, s1

	//00: Universal push-pull output mode
	c.sw s1, CFGLR_OFFSET(a5)

	la t2, test_memory
	sw s1, 0(t2)
	

	c.li s1, 0
	li s0, 0b10000000
	c.j send_inner_loop

load_next_byte:
	lb s0, 0(a0) //replace me with the QingKe v2 "special" opcode for this.
	c.addi a0, 1
	c.beqz a1, done_sending_data
	c.nop
	c.nop
	c.nop
	c.nop
send_inner_loop:
	andi a3, s0, 1

	// Handle CRC.
	c.xor a3, a2;
	c.andi a3, 1;
	c.addi a3, -1;
	and a3, a3, t0;
	c.srli a2, 1;
	c.xor a2, a0

	andi a3, s0, 1

	// Now compare the values from this to last.
	andi a3, s0, 1
	c.xor a3, s1
	c.andi a3, 1
	ori s1, s1, 1 // will be reset if a 0 bit.
	c.beqz a3, send_one_bit
send_zero_bit:
	c.andi s1, 2
	c.mv a3, s1
	c.andi a3, 2
	li a1, (1<<USB_DP) | (1<<(USB_DM+16));
	c.beqz a3, last_bit_was_a_physical_zero
	// Last bit was a physical 1.
	li a1, (1<<USB_DM) | (1<<(USB_DP+16));
last_bit_was_a_physical_zero:
	c.sw a1, BSHR_OFFSET(a5)
	li t1, 7
send_one_bit:
	addi t1, t1, -1
	beqz t1, insert_stuffed_bit
send_end_bit_complete:
	c.addi a1, -1
	andi a3, a1, 7
	c.beqz a3, load_next_byte
	// Wait an extra 4-5 cycles.
	c.j send_inner_loop

done_sending_data:
	c.bnez a2, poly_function


	lw s1, CFGLR_OFFSET(a5)
	// Convert D+/D- into inputs.
	li a3, ~((0b11<<(USB_DM*2)) | (0b11<<(USB_DP*2)))
	and s1, a3, s1
	// 01: Floating input mode.
	li a3, ((0b01<<(USB_DM*2+2)) | (0b01<<(USB_DP*2+2)))
	or s1, a3, s1
	sw s1, CFGLR_OFFSET(a5)

	lw	s0, 0(sp)
	lw	s1, 4(sp)
	addi	sp,sp,8
	ret

poly_function:
	mv s0, a3
	c.li a1, 5
	c.j send_inner_loop


insert_stuffed_bit:
	c.nop
	c.nop
	c.nop
	c.nop
	c.nop
	c.nop
	c.nop
	c.nop
	c.nop
	c.nop
	c.nop
	c.nop
	c.nop
	c.nop
	c.nop
	c.nop
	c.nop
	c.nop
	j send_end_bit_complete
