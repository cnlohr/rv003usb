all : testblink.bin busbflash

TARGET:=testblink
CH32FUN:=../../ch32fun/ch32fun
MINICHLINK?=../../ch32fun/minichlink

ADDITIONAL_C_FILES+=
SYSTEM_C:=
EXTRA_CFLAGS:=-I../rv003usb -I../lib

busbflash : busbflash.c
	gcc -o $@ $^ -ludev -I../../ch32fun/minichlink

testflash : busbflash testblink.bin
	./busbflash

rebootandrun :
	cd .. && make && sleep 1 && cd configurebootloader && make testflash

# Uncomment if you actually want to write to bootloader,
# Otherwise just a test.
LINKER_SCRIPT:=scratchpad.ld
WRITE_SECTION:=bootloader
FLASH_COMMAND:=./busbflash

include ../../ch32fun/ch32fun/ch32fun.mk

flash : busbflash cv_flash
clean : cv_clean
	rm -rf busbflash


