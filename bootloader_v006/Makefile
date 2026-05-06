all : flash

TARGET:=bootloader
CH32FUN:=../ch32fun/ch32fun
TARGET_MCU:=CH32V006

ADDITIONAL_C_FILES+=../rv003usb/rv003usb.S ../rv003usb/rv003usb.c
EXTRA_CFLAGS:=-I. -I../lib -I../rv003usb

LINKER_SCRIPT:=ch32v006fun-usb-bootloader.ld
WRITE_SECTION:=bootloader
FLASH_COMMAND:=$(CH32FUN)/../minichlink/minichlink -a -w $(TARGET).bin $(WRITE_SECTION) -B

include $(CH32FUN)/ch32fun.mk
# Chose one of these:
# OPTION_BYTE:=a55aff00# Default
OPTION_BYTE:=a55aff00# Enable bootloader, disable RESET
# OPTION_BYTE:=a55aff00# Enable bootloader, enable RESET
# OPTION_BYTE:=a55aff00# Disable bootloader, disable RESET
# OPTION_BYTE:=a55aff00# Disable bootloader, enable RESET

optionbytes :
	$(MINICHLINK)/minichlink -w +$(OPTION_BYTE) option

flash : cv_flash
clean : cv_clean


