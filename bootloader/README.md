# Bootloader

The CH32V003 features 1920 byte additional space for an optional bootloader that runs before your regular code.

This code implements a bootloader that enumerates as USB HID device (so no drivers are needed) and allows for flashing the regular code via USB using a custom version of [minichlink](https://github.com/cnlohr/ch32v003fun/tree/master/minichlink). This means you need a dedicated programmer like the Link-E only once to flash the bootloader. After that, firmware updates can simply be flashed via USB.

By now, you can't update the bootloader using itself and minichlink - you can only flash regular user code.

By default, the bootloader is active for the first ~5s after power-up, then user-code is started. But this boot-mode is highly configurable and can also be triggered by a boot button or USB Host detection instead.

## Configuring

Most CH32V003's should come from the factory by default booting to the bootloader, but you may need to set fuses. You can run the app in `configurebootloader` first to configure your chip to use the bootloader.

## Compiled size
When you compile the bootloader, you'll notice that its size is almost 1920 Bytes and thus fills the available space almost completely. The following configuration changes, but also even just the GPIO pin numbers used, change the size of the compiled code and may cause it to exceed the 1920 bytes. So you need to do some trade-offs with the overall pin and boot-mode configuration!

## Configuration

The configuration is done via #defines in `bootloader.c`. See the file for all possible configurations.

### Boot-Modes

#### Timeout

By default, the bootloader is active on every power-up for ~5s and then boots user code if no communication was present. The timeout can be configured.

~~~c
// Basic timout on power-up
#define BOOTLOADER_TIMEOUT_PWR 67 // 67 ticks ~= 5s
~~~

#### USB Host detection and timout

The bootloader can detect the presence of a USB host (not a charger) and adapt the timout in that case. This is useful for devices that are often switched on but usually not connected to a USB Host (e.g. only for firmware updated). Here you want a really short timeout for the bootloader that is only extended if a USB host is detected, giving you a fast/responsive user experience when the devices is powerd on via a charger or battery.

~~~c
// Stay in bootloader forever if USB host is detected within 3 ticks (~225ms) after power-up, otherwise boot user code
#define BOOTLOADER_TIMEOUT_PWR 3
#define BOOTLOADER_TIMEOUT_USB 0
~~~

~~~c
// Stay in bootloader for 130 ticks (~10s) if USB host is detected within 3 ticks (~225ms) after power-up, otherwise boot user code
#define BOOTLOADER_TIMEOUT_PWR 3
#define BOOTLOADER_TIMEOUT_USB 130
~~~

#### Boot-Button

A button (any high/low signal) can be used to trigger the bootloader on power-up. The GPIO for the button can be configured, as well as the trigger level and optional internal pull-up/down resistor.
Using a button you have to press while powering on the device is usually the best option for devices that are powerd by and regularly used with USB host ports - e.g. devices implementing USB HID functionality.

~~~c
// Boot-Button connected between GND and D2
#define BOOTLOADER_BTN_PORT D
#define BOOTLOADER_BTN_PIN 2
#define BOOTLOADER_BTN_TRIG_LEVEL 0 // 1 = HIGH; 0 = LOW
#define BOOTLOADER_BTN_PULL 1 // 1 = Pull-Up; 0 = Pull-Down; Optional, comment out for floating input
// BOOTLOADER_TIMEOUT_PWR 67
~~~

While you can, in theory, combine the boot button with the timeout and even USB host detection, this will cause the bootloader to exceed its size and result in a compile error. So disabling the timout by commenting out `BOOTLOADER_TIMEOUT_PWR` is recommended.

### USB Pins
As for all RV003USB projects, the pins are configured in `usb_config.h` - see [main Readme](../) for this. However, there are additional considerations to take into account:

1. As explaned above, the GPIOs used may change the compiled code size. If you need a feature like the USB Host detection, but the code gets too big, try changing the GPIOs.
2. Not configuring `USB_PIN_DPU` saves additional bytes but requires you to power the pull-up on D- via 3V3 constantly (or other means) to get enumerated.
3. Having the Bootloader Button on the same port as the USB saves additional bytes
4. If your regular user code does not use the USB stack, you may want to be able to turn off the pull-up on D-. This causes the device to disconnect properly. Otherwise the USB device will keep sowing up but be unresponsive.
5. If your user code also uses the USB stack, you must force a re-enumeration after switching from bootloader to usercode by pulsing D- low. This is easy with `USB_PIN_DPU` used. But with the pull-up fixed to 3V3, a re-enumeration can still be triggered by forcing `USB_DM` low for a moment before initializing USB in user code. But you really want to add the 33 ohm in series resistors in that case.

## USB Troubleshooting

The bootloader should enumerate as HID device with `VID:1209` and `PID:B003` by default. Use `lsusb` on linux/mac or `wmic path Win32_PnPEntity where "DeviceID like 'USB%'" get Caption, DeviceID` on windows to list USB devices. On Windows [USBLogView](https://www.nirsoft.net/utils/usb_log_view.html) and [USBView](https://learn.microsoft.com/windows-hardware/drivers/debugger/usbview) are also helpful tools.
