# ch32v003 programmer for WCH RiscV chips

While this is in `master`, it should still be considered somewhat experimental.  It may make sense to instead of use two pins, use two timers on a single pin.

This is still a bit of an RFC and should not be considered the "real path" moving forward.

## A minichlink-compatible programmer

Simply use a CH32V003, connected as follows:

![Schematic](schematic.png)

And you can program, semihost and run basic GDB on a variety of WCH chips including the CH32V003, 00x, 20x, 30x, x03x, CH57x, CH585, CH59x.

Tested and not supported: CH32V103, CH582/3. Other CH5xx riscv chips may work, but weren't tested.

It's actually only a little slower than the normal programmer somehow, in spite of bit banging USB?
