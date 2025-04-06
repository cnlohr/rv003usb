// Test

#include "ch32fun.h"
#include "rv003usb.h"

#define CFGLR_OFFSET 0
#define INDR_OFFSET 8
#define BSHR_OFFSET 16


#define LOCAL_CONCAT(A, B) A##B
#define LOCAL_EXP(A, B) LOCAL_CONCAT(A,B)

#define SYSTICK_CNT 0xE000F008

// This is 6 * n + 3 cycles
#define nx6p3delay( n, freereg ) li freereg, ((n)+1); 1: c.addi freereg, -1; c.bnez freereg, 1b

//See RV003USB_DEBUG_TIMING note in .c file.
#if defined( RV003USB_DEBUG_TIMING ) && RV003USB_DEBUG_TIMING
#define DEBUG_TICK_SETUP la x4, (TIM1_BASE + 0x24) 	// for debug
#define DEBUG_TICK_MARK .balign 4; sw x0, 0(x4)
#define	RESTORE_DEBUG_MARKER(x) lw	x4, x(sp)
#define SAVE_DEBUG_MARKER(x)	sw	x4, x(sp)
#else
#define DEBUG_TICK_SETUP la x4, (TIM1_BASE + 0x58) 	// for debug (Go nowhere)
#define DEBUG_TICK_MARK .balign 4; sw x0, 0(x4)
#define	RESTORE_DEBUG_MARKER(x) lw	x4, x(sp)
#define SAVE_DEBUG_MARKER(x)	sw	x4, x(sp)
#endif

.global test_memory
.global rv003usb_internal_data
.global rv003usb_handle_packet
.global usb_send_data
.global usb_send_empty
.global main
.global always0

/* Register map
	zero, ra, sp, gp, tp, t0, t1, t2
Compressed:
	s0, s1,	a0, a1, a2, a3, a4, a5
*/

.section .text.vector_handler
.global EXTI7_0_IRQHandler

.balign 4
EXTI7_0_IRQHandler:
	addi	sp,sp,-80
	sw	a0, 0(sp)
	sw	a5, 20(sp)
	la a5, USB_GPIO_BASE
	c.lw a0, INDR_OFFSET(a5) // MUST check SE0 immediately.
	c.andi a0, USB_DMASK

	sw	a1, 4(sp)
	sw	a2, 8(sp)
	sw	a3, 12(sp)
	sw	a4, 16(sp)
	sw	s1, 28(sp)

#if defined (RV003_ADD_EXTI_MASK) && defined (RV003_ADD_EXTI_HANDLER)
	la a1, 0x40010414 //	R32_EXTI_INTFR
	c.lw a2, 0(a1)
	c.andi a2, USB_DMASK
	c.beqz a2, another_interrupt_check
#endif

	SAVE_DEBUG_MARKER( 48 );
	DEBUG_TICK_SETUP
	c.lw a1, INDR_OFFSET(a5)
	c.andi a1, USB_DMASK;

	// Finish jump to se0
	c.beqz a0, handle_se0_keepalive

	c.lw a0, INDR_OFFSET(a5); c.andi a0, USB_DMASK; bne a0, a1, syncout
	c.lw a0, INDR_OFFSET(a5); c.andi a0, USB_DMASK; bne a0, a1, syncout
	c.lw a0, INDR_OFFSET(a5); c.andi a0, USB_DMASK; bne a0, a1, syncout
	c.lw a0, INDR_OFFSET(a5); c.andi a0, USB_DMASK; bne a0, a1, syncout
	c.lw a0, INDR_OFFSET(a5); c.andi a0, USB_DMASK; bne a0, a1, syncout
	c.lw a0, INDR_OFFSET(a5); c.andi a0, USB_DMASK; bne a0, a1, syncout
	c.lw a0, INDR_OFFSET(a5); c.andi a0, USB_DMASK; bne a0, a1, syncout
	c.j syncout
syncout:
	sw	s0, 24(sp)
	li a2, 0
	sw	t0, 32(sp)  // XXX NOTE: This is actually unused register - remove some day?
	sw	t1, 36(sp)

	// We are coarsely sync'd here.

	// This will be called when we have synchronized our USB.  We can put our
	// preamble detect code here.  But we have a whole free USB bit cycle to
	// do whatever we feel like.

	// A little weird, but this way, the USB packet is always aligned.
#define DATA_PTR_OFFSET (59+4)
	
	// This is actually somewhat late.
	// The preamble loop should try to make it earlier.
.balign 4
preamble_loop:
	DEBUG_TICK_MARK
	c.lw a0, INDR_OFFSET(a5);
	c.andi a0, USB_DMASK;
	c.beqz a0, done_usb_message // SE0 here?
	c.xor a0, a1;
	c.xor a1, a0; // Recover a1.
	j 1f; 1: // 4 cycles?
	c.beqz a0, done_preamble
	j 1f; 1: // 4 cycles?
	c.lw s0, INDR_OFFSET(a5);
	c.andi s0, USB_DMASK;
	c.xor s0, a1

	// TRICKY: This helps retime the USB sync.
	// If s0 is nonzero, then it's changed (we're going too slow)
	c.bnez s0, 2f;  // This code takes 6 cycles or 8 cycles, depending.
	c.j 1f; 1:
	2:
	j preamble_loop // 4 cycles
.balign 4
done_preamble:
	sw  t2, 40(sp)
	sw  ra, 52(sp)
	// 16-byte temporary buffer at 56+sp

	// XXX TODO: Do one byte here to determine the header byte and from that set the CRC.
	c.li s1, 8

	// This is the first bit that matters.
	c.li s0, 6 // 1 runs.

	c.nop; 

	// 8 extra cycles here cause errors.
	// -5 cycles is too much.
	// -4 to +6 cycles is OK

	//XXX NOTE: It actually wouldn't be too bad to insert an *extra* cycle here.

	/* register meanings:
		* x4 = TP = used for triggering debug.

		* T0 = Totally unushed.
		* T1 = TEMPORARY
		* T2 = Pointer to the memory address we are writing to.

		* A0 = temp / current bit value.
		* A1 = last-frame's GPIO values.
		* A2 = The running word 
		* A3 = Running CRC
		* a4 = Polynomial
		* A5 = GPIO Offset

		* S0 = Bit Stuff Place
		* S1 = # output bits remaining.
*/

.balign 4
packet_type_loop:
	// Up here to delay loop a tad, and we need to execute them anyway.
	// TODO: Maybe we could further sync bits here instead of take up time?
	// I.e. can we do what we're doing above, here, and take less time, but sync
	// up when possible.
	li a3, 0xffff // Starting CRC of 0.   Because USB doesn't respect reverse CRCing.
	li a4, 0xa001
	addi  t2, sp, DATA_PTR_OFFSET //rv003usb_internal_data
	la  t0, 0x80
	c.nop

	DEBUG_TICK_MARK
	c.lw a0, INDR_OFFSET(a5);
	c.andi a0, USB_DMASK;
	c.beqz a0, done_usb_message // Not se0 complete, that can't happen here and be valid.
	c.xor a0, a1;
	c.xor a1, a0; // Recover a1, for next cycle
	// a0 = 00 for 1 and 11 for 0

	// No CRC for the header.
	//c.srli a0, USB_PIN_DP
	//c.addi a0, 1 // 00 -> 1, 11 -> 100
	//c.andi a0, 1 // If 1, 1 if 0, 0
        c.nop
        seqz a0, a0

	// Write header into byte in reverse order, because we can.
	c.slli a2, 1
	c.or a2, a0

	// Handle bit stuffing rules.
	c.addi a0, -1 // 0->0xffffffff 1->0
	c.or s0, a0
	c.andi s0, 7
	c.addi s0, -1
	c.addi s1, -1
	c.bnez s1, packet_type_loop

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////	

	// XXX Here, figure out CRC polynomial.

	li s1, (USB_BUFFER_SIZE*8) // # of bits we culd read.

	// WARNING: a0 is bit-wise backwards here.
	// 0xb4 for instance is a setup packet.
	//
	// When we get here, packet type is loaded in A2.
	// If packet type is 0xXX01 or 0xXX11
	// the LSBs are the inverted packet type.
	// we can branch off of bit 2.
	andi a0, a2, 0x0c

	// if a0 is 1 then it's DATA (full CRC) otherwise,
	// (0) for setup or PARTIAL CRC.
	// Careful:  This has to take a constant amount of time either way the branch goes.
	c.beqz a0, data_crc
	c.li a4, 0x14	
	c.li a3, 0x1e
	.word 0x00000013 // nop, for alignment of data_crc.
data_crc:


#define HANDLE_EOB_YES \
	sb a2, 0(t2); /* Save the byte off. TODO: Is unaligned byte access to RAM slow? */ \
	.word 0x00138393; /*addi t2, t2, 1;*/

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////	
.balign 4
is_end_of_byte:
	HANDLE_EOB_YES

	// end-of-byte.
.balign 4
bit_process:
	// Debug blip
//	c.lw a4, INDR_OFFSET(a5);
	DEBUG_TICK_MARK
	c.lw a0, INDR_OFFSET(a5);
	c.andi a0, USB_DMASK;
	c.xor a0, a1;

//XXX GOOD
#define HANDLE_NEXT_BYTE(is_end_of_byte, jumptype)  \
	c.addi s1, -1; \
	andi a0, s1, 7; /* s1 could be really really big */ \
	c.jumptype a0, is_end_of_byte /* 4 cycles for this section. (Checked) (Sometimes 5)? */ 

	c.beqz a0, handle_one_bit
handle_zero_bit:
	c.xor a1, a0; // Recover a1, for next cycle

	// TODO: Do we have time to do time fixup here?
	// Can we resync time here?
	// If they are different, we need to sloowwww dowwwnnn
	// There is some free time.  Could do something interesting here!!!
	// I was thinking we could put the resync code here.
	c.j 1f; 1:  //Delay 4 cycles.

	c.li s0, 6      // reset runs-of-one.
	c.beqz a1, se0_complete

	// Handle CRC (0 bit)  (From @Domkeykong)
	slli a0,a3,31 // Put a3s LSB into a0s MSB
	c.srai a0,31    // Copy MSB into all other bits
	c.srli a3,1
	c.and  a0, a4
	c.xor  a3, a0

	c.srli a2, 1;  // shift a2 down by 1
	HANDLE_NEXT_BYTE(is_end_of_byte, beqz)
	c.nop
	c.nop
	c.nop
	c.bnez s1, bit_process // + 4 cycles
	c.j done_usb_message


.balign 4
handle_one_bit:
	c.addi s0, -1; // Count # of runs of 1 (subtract 1)
	//HANDLE_CRC (1 bit)
	andi a0, a3, 1
	c.addi a0, -1
	c.and a0, a4
	c.srli a3, 1
	c.xor a3, a0

	c.srli a2, 1;  // shift a2 down by 1
	ori a2, a2, 0x80
	c.beqz s0, handle_bit_stuff;

	HANDLE_NEXT_BYTE(is_end_of_byte, beqz)
	c.nop // Need extra delay here because we need more time if it's end-of-byte.
	c.nop
	c.nop
	c.bnez s1, bit_process // + 4 cycles
	c.j done_usb_message

handle_bit_stuff:
	// We want to wait a little bit, then read another byte, and make
	// sure everything is well, before heading back into the main loop
	// Debug blip
	HANDLE_NEXT_BYTE(not_is_end_of_byte_and_bit_stuffed, bnez)
	HANDLE_EOB_YES

not_is_end_of_byte_and_bit_stuffed:
        DEBUG_TICK_MARK
	c.lw a0, INDR_OFFSET(a5);
	c.andi a0, USB_DMASK;
	c.beqz a0, se0_complete
	c.xor a0, a1;
	c.xor a1, a0; // Recover a1, for next cycle.

	// If A0 is a 0 then that's bad, we just did a bit stuff
        //   and A0 == 0 means there was no signal transition
	c.beqz a0, done_usb_message

        // Reset bit stuff, delay, then continue onto the next actual bit
	c.li s0, 6;

        c.nop;
	nx6p3delay( 2, a0 )

	c.bnez s1, bit_process // + 4 cycles

.balign 4
se0_complete:
	// This is triggered when we finished getting a packet.
	andi a0, s1, 7; // Make sure we received an even number of bytes.
	c.bnez a0, done_usb_message



	// Special: handle ACKs?
	// Now we have to decide what we're doing based on the
	// packet type.
	addi  a1, sp, DATA_PTR_OFFSET
	XW_C_LBU(a0, a1, 0);	//lbu  a0, 0(a1)
	c.addi a1, 1

	// 0010 => 01001011 => ACK
	// 0011 => 11000011 => DATA0
	// 1011 => 11010010 => DATA1
	// 1001 => 10010110 => PID IN
	// 0001 => 10000111 => PID_OUT
	// 1101 => 10110100 => SETUP    (OK)

	// a0 contains first 4 bytes.
	la ra, done_usb_message_in // Common return address for all function calls.

	// For ACK don't worry about CRC.
	addi a5, a0, -0b01001011

	RESTORE_DEBUG_MARKER(48) // restore x4 for whatever in C land.

	la a4, rv003usb_internal_data

	// ACK doesn't need good CRC.
	c.beqz a5, usb_pid_handle_ack

	// Next, check for tokens.
	c.bnez a3, crc_for_tokens_would_be_bad_maybe_data
may_be_a_token:
	// Our CRC is 0, so we might be a token.

	// Do token-y things.
	XW_C_LHU( a2, a1, 0 )
	andi a0, a2, 0x7f // addr
	c.srli a2, 7
	c.andi a2, 0xf    // endp
	li s0, ENDPOINTS
	bgeu a2, s0, done_usb_message // Make sure < ENDPOINTS
	c.beqz a0,  yes_check_tokens
	// Otherwise, we might have our assigned address.
	XW_C_LBU(s0, a4, MY_ADDRESS_OFFSET_BYTES);	//	lbu s0, MY_ADDRESS_OFFSET_BYTES(a4)
	bne s0, a0, done_usb_message // addr != 0 && addr != ours.
yes_check_tokens:
	addi a5, a5, (0b01001011-0b10000111)
	c.beqz a5, usb_pid_handle_out
	c.addi a5, (0b10000111-0b10010110)
	c.beqz a5, usb_pid_handle_in
	c.addi a5, (0b10010110-0b10110100)
	c.beqz a5, usb_pid_handle_setup

	c.j done_usb_message_in

	// CRC is nonzero. (Good for Data packets)
crc_for_tokens_would_be_bad_maybe_data:
	li s0, 0xb001  // UGH: You can't use the CRC16 in reverse :(
	c.sub a3, s0
	c.bnez a3, done_usb_message_in
	// Good CRC!!
	sub a3, t2, a1 //a3 = # of bytes read..
	c.addi a3, 1
	addi a5, a5, (0b01001011-0b11000011)
	c.li a2, 0
	c.beqz a5, usb_pid_handle_data
	c.addi a5, (0b11000011-0b11010010)
	c.li a2, 1
	c.beqz a5, usb_pid_handle_data

done_usb_message:
done_usb_message_in:
	lw	s0, 24(sp)
	lw	s1, 28(sp)
	lw	t0, 32(sp)
	lw	t1, 36(sp)
	lw	t2, 40(sp)
	lw  ra, 52(sp)

#if defined (RV003_ADD_EXTI_MASK) && defined (RV003_ADD_EXTI_HANDLER)
another_interrupt_check:
	la a5, EXTI_BASE + 20
	lw a0, 0(a5)
	andi a0, a0, RV003_ADD_EXTI_MASK
	c.beqz a0, ret_from_se0

another_interrupt_handler:
	RV003_ADD_EXTI_HANDLER
#endif

ret_from_se0:
	lw	s1, 28(sp)
	RESTORE_DEBUG_MARKER(48)
	lw	a2, 8(sp)
	lw	a3, 12(sp)
	lw	a4, 16(sp)
	lw	a1, 4(sp)

interrupt_complete:
	// Acknowledge interrupt.
	// EXTI->INTFR = 1<<4
	c.j 1f; 1: // Extra little bit of delay to make sure we don't accidentally false fire.

	la a5, EXTI_BASE + 20
#ifdef RV003_ADD_EXTI_MASK
	li a0, (RV003_ADD_EXTI_MASK|(1<<USB_PIN_DM))
#else
	li a0, (1<<USB_PIN_DM)
#endif
	sw a0, 0(a5)
	
	// Restore stack.
	lw	a0, 0(sp)
	lw	a5, 20(sp)
	addi	sp,sp,80
	mret

///////////////////////////////////////////////////////////////////////////////
// High level functions.

#ifdef RV003USB_OPTIMIZE_FLASH
/*
void usb_pid_handle_ack( uint32_t dummy, uint8_t * data, uint32_t dummy1, uint32_t dummy2, struct rv003usb_internal * ist  )
{
	struct usb_endpoint * e = &ist->eps[ist->current_endpoint];
	e->toggle_in = !e->toggle_in;
	e->count++;
	return;
}
*/

usb_pid_handle_ack:
	c.lw a2, 0(a4) //ist->current_endpoint -> endp;
	c.slli a2, 5
	c.add a2, a4
	c.addi a2, ENDP_OFFSET // usb_endpoint eps[ENDPOINTS];

	c.lw a0, (EP_TOGGLE_IN_OFFSET)(a2) // toggle_in=!toggle_in
	c.li a1, 1
	c.xor a0, a1
	c.sw a0, (EP_TOGGLE_IN_OFFSET)(a2)

	c.lw a0, (EP_COUNT_OFFSET)(a2) // count_in
	c.addi a0, 1
	c.sw a0, (EP_COUNT_OFFSET)(a2)

	c.j done_usb_message_in

/*
//Received a setup for a specific endpoint.
void usb_pid_handle_setup( uint32_t addr, uint8_t * data, uint32_t endp, uint32_t unused, struct rv003usb_internal * ist )
{
	ist->current_endpoint = endp;
	struct usb_endpoint * e = &ist->eps[endp];

	e->toggle_out = 0;
	e->count = 0;
	e->toggle_in = 1;
	ist->setup_request = 1;
}*/
usb_pid_handle_setup:
	c.sw a2, 0(a4) // ist->current_endpoint = endp
	c.li a1, 1
	c.sw a1, SETUP_REQUEST_OFFSET(a4) //ist->setup_request = 1;
	c.slli a2, 3+2
	c.add a2, a4
	c.sw a1, (ENDP_OFFSET+EP_TOGGLE_IN_OFFSET)(a2) //e->toggle_in = 1;
	c.li a1, 0
	c.sw a1, (ENDP_OFFSET+EP_COUNT_OFFSET)(a2)  //e->count = 0;
	c.sw a1, (ENDP_OFFSET+EP_OPAQUE_OFFSET)(a2)  //e->opaque = 0;
	c.sw a1, (ENDP_OFFSET+EP_TOGGLE_OUT_OFFSET)(a2) //e->toggle_out = 0;
	c.j done_usb_message_in	

#endif

//We need to handle this here because we could have an interrupt in the middle of a control or big transfer.
//This will correctly swap back the endpoint.
usb_pid_handle_out:
	//void usb_pid_handle_out( uint32_t addr, uint8_t * data, uint32_t endp, uint32_t unused, struct rv003usb_internal * ist )	
	//sb a2, 0(a4) //ist->current_endpoint = endp;
	XW_C_SB( a2, a4, 0 ); // current_endpoint = endp
	c.j done_usb_message_in


handle_se0_keepalive:
	// In here, we want to do smart stuff with the
	// 1ms tick.

	la  a0, SYSTICK_CNT
	la a4, rv003usb_internal_data
	c.lw a1, LAST_SE0_OFFSET(a4) //last cycle count   last_se0_cyccount
	c.lw a2, 0(a0) //this cycle count
	c.sw a2, LAST_SE0_OFFSET(a4) //store it back to last_se0_cyccount
	c.sub a2, a1
	c.sw a2, DELTA_SE0_OFFSET(a4) //record delta_se0_cyccount

	li a1, 48000
	c.sub a2, a1
	// This is our deviance from 48MHz.

	// Make sure we aren't in left field.
	li a5, 4000
	bge a2, a5, ret_from_se0
	li a5, -4000
	blt a2, a5, ret_from_se0

	c.lw a1, SE0_WINDUP_OFFSET(a4) // load windup se0_windup
	c.add a1, a2
	c.sw a1, SE0_WINDUP_OFFSET(a4) // save windup

	// No further adjustments
	beqz a1, ret_from_se0

	// 0x40021000 = RCC.CTLR
	la a4, 0x40021000
	lw a0, 0(a4)
	srli a2, a0, 3 // Extract HSI Trim.
	andi a2, a2, 0b11111
	li a5, 0xffffff07  
	and a0, a0, a5	// Mask off non-HSI

	// Decimate windup - use as HSIrim.
	neg a1, a1
	srai a2, a1, 9
	addi a2, a2, 16  // add HSI offset.

	// Put trim in place in register.
	slli a2, a2, 3
	or a0, a0, a2
	sw a0, 0(a4)

	j ret_from_se0

//////////////////////////////////////////////////////////////////////////////
// SEND DATA /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


.balign 4
//void usb_send_empty( uint32_t token );
usb_send_empty:
	c.mv a3, a0
	la a0, always0
	li a1, 2
	c.mv a2, a1
//void usb_send_data( uint8_t * data, uint32_t length, uint32_t poly_function, uint32_t token );
usb_send_data:
	addi	sp,sp,-16
	sw	s0, 0(sp)
	sw	s1, 4(sp)

	la a5, USB_GPIO_BASE

	// ASAP: Turn the bus around and send our preamble + token.
	c.lw a4, CFGLR_OFFSET(a5)

	li s1, ~((0b1111<<(USB_PIN_DP*4)) | (0b1111<<(USB_PIN_DM*4)))
	and a4, s1, a4

	// Convert D+/D- into 2MHz outputs
	li s1, ((0b0010<<(USB_PIN_DP*4)) | (0b0010<<(USB_PIN_DM*4)))
	or a4, s1, a4

	li s1, (1<<USB_PIN_DP) | (1<<(USB_PIN_DM+16))
	c.sw s1, BSHR_OFFSET(a5)

	//00: Universal push-pull output mode
	c.sw a4, CFGLR_OFFSET(a5)

	li t1, (1<<USB_PIN_DP) | (1<<(USB_PIN_DM+16)) | (1<<USB_PIN_DM) | (1<<(USB_PIN_DP+16));

	SAVE_DEBUG_MARKER( 8 )

	// Save off our preamble and token.
	c.slli a3, 7     //Put token further up so it gets sent later.
	ori s0, a3, 0x40  

	li t0, 0x0000
	c.bnez a2, done_poly_check
	li t0, 0xa001
	li a2, 0xffff
done_poly_check:

	c.slli a1, 3 // bump up one extra to be # of bits
	mv t2, a1

	// t0 is our polynomial
	// a2 is our running CRC.
	// a3 is our token.
	DEBUG_TICK_SETUP

	c.li a4, 6 // reset bit stuffing.
	c.li a1, 15 // 15 bits.

	//c.nop; c.nop; c.nop;
	c.j pre_and_tok_send_inner_loop

////////////////////////////////////////////////////////////////////////////
// Send preamble + token
.balign 4
pre_and_tok_send_inner_loop:
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

	c.addi a4, -1
	c.bnez a3, pre_and_tok_send_one_bit
//pre_and_tok_send_one_bit:
//Send 0 bit. (Flip)
	// Flip s1 (our bshr setting) by xoring it.
	// 10.....01
	// 11.....11 (xor with)
	// 01.....10
	xor s1, s1, t1
	c.li a4, 6 // reset bit stuffing.
	// DO NOT flip.  Allow a4 to increment.
// Deliberately unaligned for timing purposes.
.balign 4
pre_and_tok_send_one_bit:
	sw s1, BSHR_OFFSET(a5)
	//Bit stuffing doesn't happen.
	c.addi a1, -1
	c.beqz a1, pre_and_tok_done_sending_data
	nx6p3delay( 2, a3 );	c.nop;             // Free time!
	c.j pre_and_tok_send_inner_loop
.balign 4
pre_and_tok_done_sending_data:
////////////////////////////////////////////////////////////////////////////

	// We have very little time here.  Just enough to do this.

	//Restore size.
	mv a1, t2//lw  a1, 12(sp)
	c.beqz a1, no_really_done_sending_data  //No actual payload?  Bail!
	c.addi a1, -1
//	beqz t2, no_really_done_sending_data 

	bnez t0, done_poly_check2
	li a2, 0xffff
done_poly_check2:


	// t0 is used for CRC
	// t1 is free
	// t2 is a backup of size.

	// s1 is our last "state"
	//    bit 0 is last "physical" state,
	//    
	// s0 is our current "bit" / byte / temp.

	// a0 is our data
	// a1 is our length
	// a2 our CRC
	// a3 is TEMPORARY
	// a4 is used for bit stuffing.
	// a5 is the output address.

	//xor s1, s1, t1
	//c.sw s1, BSHR_OFFSET(a5)

	// This creates a preamble, which is alternating 1's and 0's
	// and then it sets the same state.
//	li s0, 0b10000000

//	c.j send_inner_loop
.balign 4
load_next_byte:
	// CH32v003 has the XW extension.
	// this replaces: lb s0, 0(a0)
	XW_C_LBU(s0, a0, 0);
	//lb s0, 0(a0)
	//	.long 0x00150513 // addi a0, a0, 1  (For alignment's sake)
	c.addi a0, 1
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
	c.andi a3, 1
	// If a3 is 0, we should FLIP
	// if a3 is 1, we should NOT flip.
	c.beqz a3, send_zero_bit
	c.srli s0, 1 // Shift down into the next bit.
//send_one_bit:
	//HANDLE_CRC (1 bit)
	andi a3, a2, 1
	c.addi a3, -1
	and a3, a3, t0
	c.srli a2, 1
	c.xor a2, a3

	c.addi a4, -1
	c.beqz a4, insert_stuffed_bit
	c.j cont_after_jump
//Send 0 bit. (Flip)
.balign 4
send_zero_bit:
	c.srli s0, 1 // Shift down into the next bit.

	// Handle CRC (0 bit)
	// a2 is our running CRC
	// a3 is temp
	// t0 is polynomial.

	// XXX WARNING: this was by https://github.com/cnlohr/rv003usb/issues/7 
	// TODO Check me!
	slli a3,a2,31 // Put a3s LSB into a0s MSB
	c.srai a3,31    // Copy MSB into all other bits

	// Flip s1 (our bshr setting) by xoring it.
	// 10.....01
	// 11.....11 (xor with)
	// 01.....10
	xor s1, s1, t1
	sw s1, BSHR_OFFSET(a5)

	c.li a4, 6 // reset bit stuffing.

	// XXX XXX CRC down here to make bit stuffing timings line up.
	c.srli a2,1
	and a3,a3,t0
	c.xor  a2,a3 

.balign 4
cont_after_jump:
send_end_bit_complete:
	c.beqz a1, done_sending_data
	andi a3, a1, 7
	c.addi a1, -1
	c.beqz a3, load_next_byte
	// Wait an extra few cycles.
	c.j 1f; 1:
	c.j send_inner_loop

.balign 4
done_sending_data:
	// BUT WAIT!! MAYBE WE NEED TO CRC!
	beqz t0, no_really_done_sending_data
	srli t0, t0, 8 // reset poly - we don't want it anymore.
	li a1, 7 // Load 8 more bits out
	beqz t0, send_inner_loop  //Second CRC byte
	// First CRC byte
	not s0, a2 // get read to send out the CRC.
	c.j send_inner_loop


.balign 4
no_really_done_sending_data:

//	c.bnez a2, poly_function  TODO: Uncomment me!

	nx6p3delay( 2, a3 );

	// Need to perform an SE0.
	li s1, (1<<(USB_PIN_DM+16)) | (1<<(USB_PIN_DP+16))
	c.sw s1, BSHR_OFFSET(a5)

	nx6p3delay( 7, a3 );

	li s1, (1<<(USB_PIN_DM)) | (1<<(USB_PIN_DP+16))
	c.sw s1, BSHR_OFFSET(a5)

	lw s1, CFGLR_OFFSET(a5)
	// Convert D+/D- into inputs.
	li a3, ~((0b11<<(USB_PIN_DP*4)) | (0b11<<(USB_PIN_DM*4)))
	and s1, a3, s1
	// 01: Floating input mode.
	li a3, ((0b01<<(USB_PIN_DP*4+2)) | (0b01<<(USB_PIN_DM*4+2)))
	or s1, a3, s1
	sw s1, CFGLR_OFFSET(a5)

	lw	s0, 0(sp)
	lw	s1, 4(sp)
	RESTORE_DEBUG_MARKER( 8 )
	addi	sp,sp,16
	ret

.balign 4
// TODO: This seems to be either 222 or 226 (not 224) in cases.
// It's off by 2 clock cycles.  Probably OK, but, hmm.
insert_stuffed_bit:
	nx6p3delay(3, a3)
	xor s1, s1, t1
	c.li a4, 6 // reset bit stuffing.
	c.nop
	c.nop
	sw s1, BSHR_OFFSET(a5)
	c.j send_end_bit_complete

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


#ifdef USE_TINY_BOOT

// Absolutely bare-bones hardware initialization for bringing the chip up,
// setting up the environment for C, switching to 48MHz clock, and booting
// into main... as well as providing EXTI7_0_IRQHandler jump at interrupt

.section .init
.global InterruptVector
InterruptVector:
	.align  2
.option push
.option norelax
#if __GNUC__ > 10
.option arch, +zicsr
#endif
	la sp, _eusrstack  // SP = 0x20000800
	li a0, 0x80
	csrw mstatus, a0
	csrw mtvec, 3
	addi a0, sp, -2048 // A0 = 0x20000000
	addi gp, a0, 0x3fc // GP = 0x200003fc
	c.li a4, 0
1:	c.sw a4, 0(a0)	// Clear RAM
	c.addi a0, 4
	blt a0, gp, 1b  // Iterate over RAM until it's cleared.
2:
	//XXX WARNING: NO .DATA SECTION IS AVAILABLE HERE!
	/* SystemInit48HSI */

	// RCC_BASE     = 0x40021000
	// FLASH_R_BASE = 0x40022000
	// EXTEND       = 0x40023800
	la a2, RCC_BASE
	la a3, FLASH_R_BASE
	li a1, 0x00000001 | 0x01000000 | 0x80 /* RCC->CTLR RCC_HSION | RCC_PLLON | ((HSITRIM) << 3) */
	c.sw a1, 0(a2)
	c.li a1, 0x01       /* FLASH_ACTLR_LATENCY_1 */
	c.sw a1, 0(a3)      /* FLASH->ACTLR = FLASH_ACTLR_LATENCY_1 */
	c.li a1, 0x00000002 /* RCC->CFGR0 = RCC_SW_PLL */
	c.sw a1, 4(a2)
	la a1, main
	csrw mepc, a1
.option pop
	mret

// CAREFUL THIS MUST BE EXACTLY AT 0x50
. = 0x52 // Weird...  I don't know why this has to be 0x52, for it to be at 0x50.
	.word EXTI7_0_IRQHandler            /* EXTI Line 7..0 */
always0:
	.byte 0x00 // Automatically expands out to 4 bytes.

.align 0
.balign 0

#else

.balign 4
always0:
	.word 0x00

#endif
