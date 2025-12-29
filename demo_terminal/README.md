# Example on how to use USB terminal functionality on CH32V003

The idea is to use the same ``printf`` function that is used to print to minichlink's terminal via SWIO pin but with USB. 
Provided example would output both to minichlink and [WebLink](https://subjectiverealitylabs.com/WeblinkUSB/)'s terminal (not at the same time ideally).

## To try

Edit ``usb_config.h`` to match your GPIO settings for USB;

``make flash``

``make terminal_usb`` or ``../ch32v003fun/minichlink -kT -c 0x1209d003`` (VID=1209, PID=d003).

and use it like normal minichlink terminal.

If you're on linux, you may need to add udev rule:

```
echo -ne "KERNEL==\"hidraw*\", SUBSYSTEM==\"hidraw\", MODE=\"0664\", GROUP=\"plugdev\"\n" | sudo tee /etc/udev/rules.d/20-hidraw.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

## To use

in your own project, assuming you already using rv003usb, you need to add to your ``usbconfig.h``:

``#define RV003USB_USB_TERMINAL 1``

Then you need to add
```C
  HID_REPORT_COUNT   ( 7 ), // For use with `hidapitester --vidpid 1209/D003 --open --read-feature 171`
  HID_REPORT_ID      ( 0xfd )
  HID_USAGE          ( 0x01 ),
  HID_FEATURE        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,
```
to ``special_hid_desc`` struct in ``usbconfig.h`` before ``HID_COLLECTION_END``.

Also enable debugprintf in ``funconfig.h``:

``#define FUNCONF_USE_DEBUGPRINTF 1``


## How does it work

By default ``printf`` uses a single wire programming interface to get those strings to the ``minichlink``'s terminal. You don't need to attach UART and use additional pins for it. Also it's fast.

CH32V003 has a built-in Debug Module (further referenced as DM) that is used for programming and debugging via a compatible programmer. ``printf`` that is used by default in ch32v003fun leverages this DM to implement communication between the MCU and a host PC. How does it do this? It uses two debug registers ``DMDATA0`` and ``DMDATA1`` to write data to and read from.

Now, what I thought, we could just read ``DMDATA0/1`` internally and send the contents of it via USB HID to a compatible terminal. Then we won't change any logic but only enhance it with our little addition. The pros of doing it this way are that we have both interface useful within same firmware with very little added code, and it becomes mostly portable.

But there is a caveat, I've found once I started to tinker with this idea. DM in CH32V003 is an external module from the perspective of the core that is running our firmware. Until the specific command is sent to the DM via SWIO from a programmer we can't access ``DMDATA0/1`` from the firmware itself. It's not a big deal when used with a programmer, it does everything automatically during interface initialization. But we want to use it without a programmer, only with USB attached.

Can we somehow unlock the DM from inside the firmware? I would like to find a way to access the DM or its registers from the core itself, but after hours of reading manuals, datasheets, and experimenting with code I haven't found anything. If you know a way, please tell me! The only way left is to try to self-program via bit-banging SWIO pin from inside. But! The SWIO pin by default can be used only as an input and is reserved for programming purposes. We can switch it to be a plain GPIO but then it is detached from DM and can't be used to communicate with it. 

So here is a dilemma, but a physical world with its rules comes to a rescue! What we can do is: *Make SWIO GPIO > Turn it 0/1 > Make SWIO programmable pin again > Repeat as needed*. Once the unlock sequence is sent to DM it lets our firmware access both ``DMDATA0/1`` and do all we want.
