# Using USB on the CH32V003
(No, that's not a typo)

Bit banged USB stack for the RISC-V WCH CH32V003, using [ch32v003fun](https://github.com/cnlohr/ch32v003fun).

### WARNING: Project is not ready for prime time.  Proof of concept works, we are on track for proper releases soon.  Maybe you can help out!

:white_check_mark: Able to go on-bus and receive USB frames  
:white_check_mark: Compute CRC in-line while receiving data  
:white_check_mark: Bit Stuffing (Works, tested)  
:white_check_mark: Sending USB Frames  
:white_check_mark: High Level USB Stack in C  
:white_check_mark: Make USB timing more precise.  
:white_check_mark: Use SE0 1ms ticks to tune HSItrim  
:white_check_mark: Rework sending code to send tokens/data separately.  
:white_check_mark: Fix CRC Code  
:grey_exclamation: Further optimize Send/Receive PHY code. (Please help)  
:negative_squared_cross_mark: Enable systick-based retiming to correct timing mid-frame.  
:white_check_mark: Optimize high-level stack.  
:white_check_mark: Fit in bootloader (NOTE: Need to tighten more)  
:grey_exclamation: Use hardware CRC to save space/time??  
:white_check_mark: Use HID custom messages.  
:grey_exclamation: Improve sync sled.  I.e. coarse and fine sledding.  
:white_square_button: Abort on non-8-bit-aligned-frames.  
:white_square_button: Make minichlink able to use bootloader.  
:warning: Arduino support (someone else will have to take this on)  
:white_square_button: Make demos  
:white_square_button: API For self-flashing + printf from bootloader  
:warning: Enable retiming (Requires a few more cycles, will be major effort)  

