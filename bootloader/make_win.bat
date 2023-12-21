REM Bad USB Operation on pgm-wch-linke.c ? Zadig!
del bootloader.bin
del bootloader.elf
del bootloader.hex
del bootloader.lst
del bootloader.map
make build
..\ch32v003fun\minichlink\minichlink.exe -a -w bootloader.bin bootloader -B