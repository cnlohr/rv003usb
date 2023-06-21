# Using USB on the CH32V003

Ever wanted a 10 cent USB-capable processor? Well, look no further. RV003USB allows you to, in firmware connect a 10 cent RISC-V to a computer with USB. It is fundamentally just a pin-change interrupt routine that runs in assembly, and some C code to handle higher level functionality.

### It's small!

The bootloader version of the tool can create a HID device, enumerate and allow execution of code on-device in 1,920 bytes of code.

### It's simple!

The core assembly code is only 2 functions, an [interrupt](https://github.com/cnlohr/rv003usb/blob/master/rv003usb/rv003usb.S#L43) and [code to send](https://github.com/cnlohr/rv003usb/blob/master/rv003usb/rv003usb.S#L547). 

### It's adaptable!

The [core assembly](https://github.com/cnlohr/rv003usb/blob/master/rv003usb/rv003usb.S) code is standardized, but the C code, to handle different functionality with hid feature requests, control setup events, interrupt endpoints (send and receive) are all done in [a C file that changes per your needs](https://github.com/cnlohr/rv003usb/blob/master/demo_gamepad/rv003usb.c).

### It's built on ch32v003fun

[ch32v003fun](https://github.com/cnlohr/ch32v003fun) is a minimal development SDK of sorts for the CH32V003, allowing for maximum flexability without needing lots of code surrounding a HAL.

### It's still in alpha.

This project is not ready for prime time, though it is sort of in an early alpha phase.  Proof of concept works, we are on track for proper releases soon.  Maybe you can help out!

:white_check_mark: Able to go on-bus and receive USB frames  
:white_check_mark: Compute CRC in-line while receiving data  
:white_check_mark: Bit Stuffing (Works, tested)  
:white_check_mark: Sending USB Frames  
:white_check_mark: High Level USB Stack in C  
:white_check_mark: Make USB timing more precise.  
:white_check_mark: Use SE0 1ms ticks to tune HSItrim  
:white_check_mark: Rework sending code to send tokens/data separately.  
:white_check_mark: Fix CRC Code  
:white_check_mark: Make minichlink able to use bootloader.  
:white_check_mark: Optimize high-level stack.  
:white_check_mark: Fit in bootloader (NOTE: Need to tighten more)  
:white_check_mark: Do basic retiming, at least in preamble to improve timing.  
:white_check_mark: Use HID custom messages.  
:white_check_mark: Improve sync sled.  I.e. coarse and fine sledding.  
:white_check_mark: Abort on non-8-bit-aligned-frames.  
:white_square_button: Make more demos  
:white_square_button: API For self-flashing + printf from bootloader  
:white_square_button: Improve timing on send, for CRC bits.  Currently we are off by about 6 cycles total.  
:grey_exclamation: Use hardware CRC to save space/time (Investigate)  
:grey_exclamation: Further optimize Send/Receive PHY code. (Please help)  
:warning: Enable improved retiming (Requires a few more cycles) (Please help!)  
:warning: Arduino support (someone else will have to take this on)  

