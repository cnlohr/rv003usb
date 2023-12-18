# Custom Device

Have you ever wanted to have a USB device you could talk to from userspace programs on Windows and Linux?  Well, do I have the demo for you.

This demo sets up a device with a HID descriptor on endpoint 1.  But we don't actually use that endpoint for anything.  Everything can be done with control (feature) messages, which don't have the same bandwidth restrictions as interrupt requests.

Control messages are what you use for `hid_send_feature_report` and `hid_get_feature_report`.  These are advanteagous because they let you transfer data very fast.  Usually at > 500kbit/s.  They also handle framing messages, so you don't need to tell either side how big your messages are, use escape sequences, etc.  They just transfer a block of data from one side to the other.

There is a demo in 'testtop' that shows how to use hidapi to comm to the part.

This device sets up a scratchpad that the host PC is allowed to talk write to and read from.  So this demo app just opens the USB device, writes random data into the scratch pad and reads it back to make sure the data was read back OK.

There are two features, 0xAA (170), which is 255 (254) bytes in size.  As well as 0xAB, which is 64 (63) bytes in size.  These can be tested with hidapitester. `hidapitester --vidpid 1209/D003 --open --read-feature 171`

## Gotchas

So you are aware, there are a number of gotchas when making platform-agostic HID RAW devices in Linux
 * The first byte of the requests must be the feature ID. (this demo uses 0xaa)
 * Windows REQUIRES notification in the HID descritor of the feature match both ID and transfer size!
 * Yes, that's annoying because it means on Windows at least all your messages must be the same size.
 * On Linux, you may need to either be part of plugdev, and/or have your udev rules include the following:
```
KERNEL=="hidraw*", SUBSYSTEM=="hidraw", MODE="0664", GROUP="plugdev"
```
