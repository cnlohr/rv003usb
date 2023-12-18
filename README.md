# Using USB on the CH32V003

Ever wanted a 10 cent USB-capable processor? Well, look no further. RV003USB allows you to, in firmware connect a 10 cent RISC-V to a computer with USB. It is fundamentally just a pin-change interrupt routine that runs in assembly, and some C code to handle higher level functionality.

### It's small!

The bootloader version of the tool can create a HID device, enumerate and allow execution of code on-device in 1,920 bytes of code.  Basic HID setups are approximately 2kB, like the Joystick demo.

### It's simple!

The core assembly code is very basic, having both an [interrupt](https://github.com/cnlohr/rv003usb/blob/master/rv003usb/rv003usb.S#L43) and [code to send](https://github.com/cnlohr/rv003usb/blob/master/rv003usb/rv003usb.S#L547).   And only around [250 lines of C](https://github.com/cnlohr/rv003usb/blob/master/rv003usb/rv003usb.c) code to facilitate the rest of the USB protocol.    

### It's adaptable!

The [core assembly](https://github.com/cnlohr/rv003usb/blob/master/rv003usb/rv003usb.S) code is standardized, and there is also [a c file](https://github.com/cnlohr/rv003usb/blob/master/rv003usb/rv003usb.c) code, to handle different functionality with hid feature requests, control setup events, interrupt endpoints (send and receive) are all done in.

It shows both how to be a normal USB device, as well as how to write programs to run on your PC that can talk to your USB device.

### It requires very little hardware!

![Example Schematic](https://raw.githubusercontent.com/cnlohr/rv003usb/master/doc/schematic.png)

The reason why D+/D- is "flipped" here verses `usb_config.h` is because for USB low-speed, the D+/D- lines are swapped. It is frustratingly unintuitive.

As-written you **cannot** use PD6,7 or PC6,7 in any combination with the USB stack.  You must use other pins.  Sorry, let me know if you find a way.

 * `U1` is a CH32V003
 * `J1` is a USB type C connector.
 * `R1` 1.5k 5% is required under all configurations, though it may connect† D- to DPU or VCC if the intent is for the part to always be on-bus.
 * `U2`, `C1`, `C2` are used to run the CH32V003 at 3.3V.  For USB it should not be run beyond 3.6V.
 * `R2`, `R3` 5.1k resistors are only needed if using a type C connector.  If using a USB C host the host will not provide power without these.

It even supports the SOIC-8 - see this thread: https://github.com/cnlohr/rv003usb/issues/25#issuecomment-1779130408

Optionally 33 or 47 ohm resistors may be placed in-series with D+ and D- to the port to reduce noise and help protect the chip.

† If you initialize bus resets by holding the pins low, it is strongly recommended to add the afore mentioned resistors.

### It's got demos!

Here are the following demos and their statuses...

| Example      | Description | ❖ | 🐧 |
| ------------ | ----------- | --------------- | ------------- |
| [demo_gamepad](https://github.com/cnlohr/rv003usb/tree/master/demo_gamepad) | Extremely simple example, 2-axis gamepad + 8 buttons | `✅` | `✅` |
| [demo_composite_hid](https://github.com/cnlohr/rv003usb/tree/master/demo_composite_hid) | Mouse and Keyboard in one device | `✅` | `✅` |
| [demo_hidapi](https://github.com/cnlohr/rv003usb/tree/master/demo_hidapi) | Write code to directly talk to your project with fully-formed messages. (Works on [Android](https://github.com/cnlohr/androidusbtest)) | `✅` | `✅` |
| [bootloader](https://github.com/cnlohr/rv003usb/tree/master/bootloader) | CH32V003 Self-bootloader, able to flash itself with minichlink | `✅` | `✅` |

And the following largely incomplete, but proof-of-concept projects:

| Example      | Description | ❖ | 🐧 |
| ------------ | ----------- | --------------- | ------------- |
| [cdc_exp](https://github.com/cnlohr/rv003usb/tree/master/testing/cdc_exp) | Enumerate as a USB Serial port and send and receive Data (incomplete, very simple) | `⚠️` | `✅` | :question: |
| [demo_midi](https://github.com/cnlohr/rv003usb/tree/master/testing/demo_midi) | MIDI-IN and MIDI-OUT | `✅` | `✅` |
| [test_ethernet](https://github.com/cnlohr/rv003usb/tree/master/testing/test_ethernet) | RNDIS Device (note: VERY SLOW) | `⚠️` | `✅` |

Note: CDC In windows likely CAN work, but I can't figure out how to do it.  Linux explicitly blacklists all low-speed USB Ethernet that I could find.  The MIDI example only demonstrates MIDI-OUT.

### It's built on ch32v003fun

[ch32v003fun](https://github.com/cnlohr/ch32v003fun) is a minimal development SDK of sorts for the CH32V003, allowing for maximum flexability without needing lots of code surrounding a HAL.

## General developer notes

### Care surrounding interrupts and critical sections.

You are allowed to use interrupts and have critical sections, however, you should keep critical sections to approximately 40 or fewer cycles if possible.  Additionally if you are going to be using interrupts that will take longer than about 40 cycles to execute, you must enable preemption on that interrup.  For an example of how that works you can check the ws2812b SPI DMA driver in ch32v003fun.  The external pin-chane-interrupt **must** be the highest priority. And it **must never** be preempted.  While it's OK to have a short delay before it is fired, interrupting the USB reception code mid-transfer is prohibited.

## It's still in beta.

This project is not ready for prime time, though it is sort of in a beta phase.  Proof of concept works, lots of demos work.  Maybe you can help out!

`✅` Able to go on-bus and receive USB frames  
`✅` Compute CRC in-line while receiving data  
`✅` Bit Stuffing (Works, tested)  
`✅` Sending USB Frames  
`✅` High Level USB Stack in C  
`✅` Make USB timing more precise.  
`✅` Use SE0 1ms ticks to tune HSItrim  
`✅` Rework sending code to send tokens/data separately.  
`✅` Fix CRC Code  
`✅` Make minichlink able to use bootloader.  
`✅` Optimize high-level stack.  
`✅` Fit in bootloader (NOTE: Need to tighten more)  
`✅` Do basic retiming, at least in preamble to improve timing.  
`✅` Use HID custom messages.  
`✅` Improve sync sled.  I.e. coarse and fine sledding.  
`✅` Abort on non-8-bit-aligned-frames.  
`🔳` Make more demos  
`🔳` API For self-flashing + printf from bootloader  
`🔳` Improve timing on send, for CRC bits.  Currently we are off by about 6 cycles total.  
`❕` Further optimize Send/Receive PHY code. (Please help)  
`⚠️` Enable improved retiming (Requires a few more cycles) (Please help!)  
`⚠️`  Arduino support (someone else will have to take this on)  

