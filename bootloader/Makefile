all : flash

TARGET:=rv003usb
CH32V003FUN:=../ch32v003fun/ch32v003fun
MINICHLINK?=../ch32v003fun/minichlink

ADDITIONAL_C_FILES+=rv003usb.S
SYSTEM_C:=

LINKER_SCRIPT:=$(CH32V003FUN)/ch32v003fun-bootloader.ld
WRITE_SECTION:=bootloader
FLASH_COMMAND:=$(CH32V003FUN)/../minichlink/minichlink -a -U -w $(TARGET).bin $(WRITE_SECTION) -B

include ../ch32v003fun/ch32v003fun/ch32v003fun.mk

flash : cv_flash
clean : cv_clean


