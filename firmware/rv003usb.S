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

/* Register map
	zero, ra, sp, gp, tp, t0, t1, t2
Compressed:
	s0, s1,	a0, a1, a2, a3, a4, a5
*/

.global EXTI7_0_IRQHandler
EXTI7_0_IRQHandler:
	addi	sp,sp,-44
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

	// r0 points to SYSTICK 
	// a5 points to our port's input.
	la  t0, SYSTICK_CNT
	la  t2, usb_buffer

	nx6p3delay( 1 ); // 15 cycles

	c.xor a1, a0 // Recover 
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
	li s1, 8 // # of bits

	c.nop; c.nop; c.nop; c.nop
	c.nop; c.nop;
handle_bit_stuff:  // I think this is where this goes.
	c.nop; c.nop;
	c.nop; c.nop; c.nop; c.nop
	c.nop; c.nop; c.nop; c.nop
	c.nop; c.nop; c.nop; c.nop
	c.nop; c.nop; c.nop; c.nop


	// This is the first bit that matters.
	li s0, 7 // 1 runs.

.align 4

bit_process:
	// XXX For some reason, this loop is 2 cycles longer than I would expect.

	lw a0, INDR_OFFSET(a5);
	c.andi a0, USB_DMASK;
	c.beqz a0, se0_complete
	c.xor a0, a1;
	c.xor a1, a0; // Recover a1, for next cycle.
	// a0 is either 00 for 1 or 11 for 0, anything else is error.
	c.mv a4, a0
	c.slli a4, 1  // a4 is 6 if it's a 0 bit.
	c.or a4, a0   // a4 is 7 if it's a 0 bit.
	c.or s0, a4   // s0 becomes 7 if it's a 0 bit.
	c.addi s0, -1 // Count # of runs of 1 (subtract 1)
	c.beqz s0, handle_bit_stuff
	xori a0, a0, 3 //00 for 0 or 11 for 1. // THIS TAKES 2 CYCLES
	c.andi a0, 1  //0 for 0 and 1 for 1.
	// a0 = 0 for a 0 bit or 1 for a 1 bit.
	// 12 cycles (I think)?

	// Handle storing the bits.
	c.slli a2, 1
	c.addi s1, -1 // # of bits left.
	c.or a2, a0
	c.andi s1, 7

// Warning - alignment here matters.
	c.bnez a1, not_end_of_byte // 4 cycles for this section. (Checked) (Sometimes 5)?
	sw a2, 0(t2) // Save the byte off. TODO: Is unaligned byte access to RAM slow?
	addi t2, t2, 1
not_end_of_byte:


	// Debug blip
	li a4, 1<<2
	sw a4, BSHR_OFFSET(a5)
	li a4, 1<<(16+2)
	sw a4, BSHR_OFFSET(a5)

	j bit_process // + 4 cycles

se0_complete:


done_usb_message:

	lw	a2, 8(sp)
	lw	a3, 12(sp)
	lw	a4, 16(sp)
	lw	s0, 24(sp)
	lw	s1, 28(sp)
	lw	t0, 32(sp)
	lw	t1, 36(sp)
	lw	t2, 40(sp)

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
	addi	sp,sp,44
	mret

handle_se0_keepalive:
	// In here, we want to do smart stuff with the
	// 1ms tick.
	j interrupt_complete


