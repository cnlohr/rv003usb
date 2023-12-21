all : flash

TARGET:=bootloader
CH32V003FUN:=../ch32v003fun/ch32v003fun
MINICHLINK?=../ch32v003fun/minichlink

ADDITIONAL_C_FILES+=../rv003usb/rv003usb.S
SYSTEM_C:=
EXTRA_CFLAGS:=-I. -I../lib -DUSE_TINY_BOOT -I../rv003usb

# Uncomment if you actually want to write to bootloader,
# Otherwise just a test.
LINKER_SCRIPT:=ch32v003fun-usb-bootloader.ld
WRITE_SECTION:=bootloader
FLASH_COMMAND:=$(CH32V003FUN)/../minichlink/minichlink -a -w $(TARGET).bin $(WRITE_SECTION) -B

include ../ch32v003fun/ch32v003fun/ch32v003fun.mk

flash : cv_flash
clean : cv_clean


