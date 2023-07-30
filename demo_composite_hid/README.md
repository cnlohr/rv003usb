# Demo Composite device

Many times it makes sense to have devices that can be multiple devices, i.e. mouse + keyboard, or even keyboard + keyboard if you want to click enough buttons!  (Note: Don't do more than 8 bytes for regular HID payloads otherwise you will crash some people's BIOSes).

This demo presents itself as BOTH a mouse (Actually a tablet) on endpoint 1 *and* a keyboard on endpoint 2.  It moves the cursor in a little box shape and presses 'b' every second or so.

