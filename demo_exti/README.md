# Additional EXTI interrupt

This is a copy of hidapi demo with additions to show how to use additional EXTI channels while using rv003usb. Software USB stack is relying heavily on external pin interrupt, this made it impossible to use EXTI for anything else. This example shows how you can add another EXTI channel with simple interrupt handler.

First you need to chose a GPIO for this. Pin number can't be the same as pin number of GPIO that is used for D- of the USB. You need to enable corresponding GPIO peripheral, EXTI channel and set Falling/Rising edge detection for that channel.

Then you need to define ``RV003_ADD_EXTI_MASK`` and ``RV003_ADD_EXTI_HANDLER`` in ``funconfig.h`` or ``usb_config.h``. ``RV003_ADD_EXTI_MASK`` should contain mask of one or more additional EXTI channels, ``RV003_ADD_EXTI_HANDLER`` is where you put your assembly code that will be called at the interrupt, if additional EXTI channel was triggered. For now there is no easy way to compile C code and put it back to .S file, and frankly you better keep the handler as simple as possible. You can use s1, a0, a2, a3, a4 and a5 registers without any additional precautions.

The demo as is uses PC0 to toggle the LED on rising edge as indication of working interrupt on PC7 - EXTI channel 7.

You can use ``testtop`` utility from ``demo_hidapi`` to test that additiona iterrupt code doesn't mess with USB functionality.