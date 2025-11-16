# Notes

This is an out of spec xinput implementation emulating an Xbox 360 controller. It mostly functions on Linux and functions partially on Windows. Buttons are pressed and axes are swept.

Xinput is a proprietary Microsoft input protocol mainly aimed at the company's Xbox series controllers. The USB protocol through which devices enumerate as Xinput devices and talk to the Xinput driver is called **XUSBI**.

Information on XUSBI can be found on Microsoft's website here: https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-xusbi/c79474e7-3968-43d1-8d2f-175d47bef43e

---

The documentation states that XUSBI supports both Low Speed and Full Speed operation. This demo attempts to implement low speed XUSBI.
Unfortunately this does not work as intended for unknown reasons. Specifically, the interfaces and endpoints were changed from their default 20 byte reports to 8 byte reports.
We split each 20 byte report into multiple messages (8+8+4). According to the USB spec this should work fine, and it does work fine under Linux.
Under Windows unfortunately only the first 8 bytes are actually handled which limits us to all buttons, the triggers and the LS X axis. The remaining 12 bytes appear to be ignored by the Windows Xinput driver.

The spec does state that low speed should be possible, but how to do this is yet to be discovered. Something worth looking at is the Wireless adapter protocol.
It is slightly different to the wired one and appears to have default report sizes of 8 bytes, so it may work if implemented, however it requires ALL optional interfaces to be implemented unlike the Wired protocol.
This means implementing battery reports, audio reports and the peripheral port reports. Additionally it may or may not need Xbox Security Method 3 to be implemented. See here for more info on XSM3: https://github.com/oct0xor/xbox_security_method_3

---

![diagram](/testing/demo_xinput/diagram.png)
