# Notes











# Historical Notes

## Livestream #1 

Let’s write a software USB (bit banged) stack.

Background
  * espusb
    * I wrote it for the ESP8266.
    * Pure Software
  * V-USB
    * Software USB for the AVR
  * Graniuum
    * ARM-specific

USB Low-Speed
  * 1.5Mbit/s
  * Differential
  * 3.3V
  * Weird bit stuffing rules
  * Multiple CRCs
  * Requires extra resistor/+GPIO

Approaches
  * Pure GPIO DMA
    * Hard-limited to 2.2MSPS
    * Clock drift will destroy you
    * Interrupt jitter will destroy you
    * Requires dedicated timer
  * Timer Capture
    * Complicated
    * Difficult to configure
    * Need to solve inverse problem
             Time deltas.
    * Requires dedicated timer
  * TC + DMA
    * See above.
    * May allow app to continue running
    * Requires dedicated timer
  * OPA
    * Helps us but requires extra pin.
  * Pure CPU
    * If DMA is ongoing will shift our timing
    * Clock drift will kill us.
  * Mixed SYSTICK/CPU
    * Is it fast enough? Probably not.
    * What about timeouts?

Hardware we will be using:
  * GPIO
  * SYSCNT
  * HISTRIM
  * 1920-byte boot ROM
  * 48MHz operation to begin with.

15:00 Starting 
2:30:00 We have bins.

## Livestream #2 (Target 5-hours)

Let's give up on fancy DMA stuff, let's try something more basic.
  *GPIO Interrupt
  *Rely on instruction timing.
  *Re-Time from STK if needed.

Post-Notes:
  *Too many things going on last time.
  *Re-Time from STK if needed

Today we will be:
  *Making notes about how long each instruction takes.
  *Reason about the upcoming approach.


This table will be:
  *Running from FLASH, with ONE EXTRA wait state (default)
  *Running at 48MHz.


This part of the test turns a GPIO on, runs some code, and turns it off.
| Definition | Time |
| --- | --- |
| One Cycle | 21ns |
| Baseline | 40ns |
| Just one NOP | 62ns |
| Store-word IOREG | 82ns |
| 2x addi (not C) (unaligned) | 104ns |
| 2x addi (not C) (aligned) | 104ns |
| c.sw (SRAM) | 82ns |
| 2x c.sw (SRAM) | 124ns |
| c.nop then sw IOREG (unaligned) | 124ns |
| c.nop then c.sw IOREG (unaligned) | 104ns |
| c.nop then sw IOREG (aligned) | 102ns |
| c.lw IOREG | 82ns |
| lw IOREG (aligned) | 82ns |
| lw IOREG (unaligned) | 104ns |
| c.lw IOREG then use data as c.addw | 102ns |
| c.bnez, branch taken | 124ns |
| c.bnez, branch not taken | 61ns |
| c.j | 124ns |
| xori AND unaligned XORI both take | 61ns |
| sb with aligned AND unaligned writing | 84ns |

Exception, maybe? unaligned `la` is still only 2 instructions?

This takes 3 cycles?
```
	slli a2, a0, 1
	or s0, a4, a0
```

To investigate:
```
5 Cycles:
	c.slli a2, 1
	c.or a2, a0
	c.addi s1, -1 // # of bits left.
	andi a4, s1, 31 // mask off so we only look at bottom 7 bits.
```

Specifically it's that last `andi` that seems to be 2 cycles.

Also, at least in some situations, xori also takes >1 cycle.

But why? It appears to have nothing to do with alignment. It even happens with slli?!

NOTE: It seems to have something to do with the alignment of nearby jumps?

TEST: Tried changing the alignment of my function, it still took as long.

I guess branching takes 3 cycles, if target is aligned, 5 cycles, else.

MAJOR NOTE:  The TARGET of a branch must be ALIGNED elss it goes much slower.

## Interrupt reception test

W/O HPE: 444ns
W/ HPE: 589ns


1:09:00 Test interrupt in assembly complete. Continuing tests.
Got a pretty good list together.
1:40:00 Starting to write sync code?

Stream 2B:

59:22 We are detecting preambles.

## Notes, prepping for Stream 3

 * Implement changes outlined in #2
 * Change around loop to be non-nested loops (Jump-back for write-byte, check
 * Turn on MCO
 * Test data in user space.
 * CRC Code

