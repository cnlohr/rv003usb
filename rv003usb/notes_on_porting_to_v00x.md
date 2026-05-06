# Notes on porting to v00x

## Initial measurements

With debug ticks enable only in ``packet_type_loop``, interval between ticks on v006 when running from RAM is 542ns, when running from flash - 958ns. The same setup on v003 running normally - 625ns. Removing one of the nops in the loop reduces sone interval to 917ns, but mostly increases to 1833ns, likely it skips bits this way. Removing both nops decreases interval to stable 833ns.

c.nop from flash ~40ns

## Flash vs RAM

Running from flash is 1.5-2 times slower on v00x compared to v003.

Running from flash doesn't seem feasible. At some point I thought I can make at least send functions work, but alas. In both receive and send parts I could make sync->packet_type_loop work nicely, but the bit loops are too complex and I couldn't make them less than ~800ns.

Running from RAM instead is much faster than both v003 and v00x running from flash. I couldn't yet build a proper instructions timing profile, so I tried to go by the feel and rough measurements that I got using tools at my disposal. What I noticed that I could generally add a fixed delay per each branching instruction (c.bnez, c.beqz, etc), to get code timing roughly where I needed. Strangely normal jumps didn't need extra delays. I also had to insert big amount of delay between certain sections of the code, to make things work, not sure why. Maybe my assumptions are very wrong, but that's what helped me to get a somewhat reliable transmission.

Running from RAM you want to save some space by selecting only the time sensitive code, so you want to move all other USB functions back to flash. This means you no longer can use normal jumps because offsets between flash and RAM is *huge* and more than 1MB that is the limit for normal jumps. You have to use ``jalr`` that takes a address to jump directly from a reg, but you also need to place it there first.

## Instruction weirdness

While I was working on this I wanted to have debug pulses only in some parts of the transmission. This is when I added ``DEBUG_TICK_MARK_DUMMY`` that was doing ``sw x0, 0(x0)`` instead of ``sw x0, 0(x4)``. I thought this will take the same time, so it wouldn't impede the general timing. Little did I know, for some reason ``sw x0, 0(x0)`` is slightly faster, and I had to fix the timings differences afterwards, when I got to the part when I was merging the v00x code with v003's.

For my extra delay I decided to use a ``addi zero, zero, 0``, once again, I'm not sure what time does it take exactly, but it feels that it's not equal to two ``c.nop`` even though logic tells me it should be. Would be cool if someone finally measured all the time differences.

## GPIO limitation can be removed?

Since we have much faster running loops in RAM, we should be able to switch to full ``lw``/``sw`` instructions to access the GPIOs beyond 4th. But this will require once again timing readjustment, though I think it is doable.

## Static rv003usb code in bootloader area

This was an idea for a long time, to put all USB functions into bootloader are with the USB bootloader itself and then link to there from the main code, to save program space. Since v00x has 3256 bytes of space there, it should be feasible now while the bootloader code is working here.