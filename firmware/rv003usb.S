// Test

#define ASSEMBLER
#include "ch32v003fun.h"
#include "rv003usb.h"

#define BSHR_OFFSET 16
#define INDR_OFFSET 8

#define SYSTICK_CNT 0xE000F008

// This is 6 * n + 3 cylces
#define nx6p3delay( n ) li s0, ((n)+1); 1: c.addi s0, -1; c.bnez s0, 1b


.global test_memory
.global usb_buffer
.global rv003usb_handle_packet

/* Register map
	zero, ra, sp, gp, tp, t0, t1, t2
Compressed:
	s0, s1,	a0, a1, a2, a3, a4, a5
*/

.global EXTI7_0_IRQHandler
EXTI7_0_IRQHandler:
	addi	sp,sp,-52
	sw	a0, 0(sp)
	sw	a5, 20(sp)
	sw	a1, 4(sp)

	la a5, GPIOD_BASE
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

	// r0 points to SYSTICK 
	// a5 points to our port's input.
	la  t0, SYSTICK_CNT
	la  t2, usb_buffer

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

	li t0, 0xA001

	// This is the first bit that matters.
	li s0, 7 // 1 runs.

	c.nop; c.nop; c.nop; c.nop;
	c.nop; c.nop; c.nop; c.nop;
	c.nop; c.nop; c.nop; c.nop;
	c.nop;  //XXX TODO: Tune?

	/* register meanings:
		* T0 = Polynomial
		* T1 = TEMPORARY

		* A0 = temp / current bit value.
		* A1 = last-frame's GPIO values.
		* A2 = The running word 
		* A3 = Running CRC
		* A4 = temp
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

	sw x0, 0(x4)
	sw x0, 0(x4)
	sw x0, 0(x4)
	sw x0, 0(x4)
	sw x0, 0(x4)
	sw x0, 0(x4)
	sw x0, 0(x4)


#define HANDLE_EOB_YES \
	sb a2, 0(t2); /* Save the byte off. TODO: Is unaligned byte access to RAM slow? */ \
	addi t2, t2, 1;

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////	

is_end_of_byte:
	HANDLE_EOB_YES

	// end-of-byte.
bit_process:
	c.lw a0, INDR_OFFSET(a5);
	c.andi a0, USB_DMASK;
	c.beqz a0, se0_complete
	c.xor a0, a1;
	c.xor a1, a0; // Recover a1, for next cycle

#define HANDLE_CRC \
	c.xor a0, a3; \
	c.andi a0, 1; \
	c.addi a0, -1; \
	and a0, a0, t0; \
	c.srli a3, 1; \
	c.xor a3, a0

#define HANDLE_NEXT_BYTE(is_end_of_byte)  \
	c.addi s1, -1; \
	andi a0, s1, 7; /* s1 could be really really big */ \
	c.beqz a0, is_end_of_byte /* 4 cycles for this section. (Checked) (Sometimes 5)? */ 


	c.bnez a0, handle_zero_bit

handle_one_bit:
	c.addi s0, -1; // Count # of runs of 1 (subtract 1)
	c.srli a2, 1;  // shift a2 down by 1

	HANDLE_CRC
	c.li a0, 1
	c.slli a0, 7
	or a2,a2, a0;   // It's a 1, or it in.
	c.beqz s0, handle_bit_stuff;
	HANDLE_NEXT_BYTE(is_end_of_byte)

	// Debug blip
	c.nop; c.nop;
	sw x0, 0(x4)
	c.bnez s1, bit_process // + 4 cycles
	c.j done_usb_message

handle_zero_bit:
	c.li s0, 7      // reset runs-of-one.
	c.srli a2, 1;   // shift a2 down by 1
	
	HANDLE_CRC
	HANDLE_NEXT_BYTE(is_end_of_byte)

	// Debug blip
	c.nop; c.nop;
	sw x0, 0(x4)

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

	lw	x4, 48(sp)
	la  a0, usb_buffer
	sub a1, t2, a0
	mv  a2, a3
	// rv003usb_handle_packet( data, length, crc );
	//call rv003usb_handle_packet

done_usb_message:

	lw	a2, 8(sp)
	lw	a3, 12(sp)
	lw	a4, 16(sp)
	lw	s0, 24(sp)
	lw	s1, 28(sp)
	lw	t0, 32(sp)
	lw	t1, 36(sp)
	lw	t2, 40(sp)
	lw	x4, 48(sp)

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
	addi	sp,sp,52
	mret

handle_se0_keepalive:
	// In here, we want to do smart stuff with the
	// 1ms tick.
	j interrupt_complete


