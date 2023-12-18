# HIDAPI Demo

This shows how to use the CH32V003 as a general-purpose HID device, with a HIDAPI endpoint that canbe used for any data you would like.

This demo sets up a HID endpoint that's 255 bytes in size.  Because it's low-speed USB, it transfers in groups of 8 bytes.

On my PC I typically get values of 50-60 KB/s (PC->003) and 64 KB/s (003->PC)

This just shows a simple ping-pong, but more advanced usage is totally possible.

