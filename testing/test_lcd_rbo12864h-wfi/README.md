# rbo12864h-wfi video driver for Linux

You can flash this firmware on a ch32v003 and hook it up to an RBO12864H-WFI.

![Display](https://github.com/cnlohr/rv003usb/blob/master/testing/test_lcd_rbo12864h-wfi/hardware/demo.jpg?raw=true)

The display is available from https://www.ronboe.com/ they are very inexpensive but you have to ask for them.

1. Optionally you can flash the bootloader so you can continue to work on this program later.
2. Flash this code to the ch32v003.
3. cd host
4. sudo ./host

The controls allow you to change the contrast, resistor ratio, backlight brightness and cutoff for black/white.

Schematics are located in the hardware/ folder.


