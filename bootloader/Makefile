all : flash

TARGET:=bootloader
CH32FUN:=../ch32v003fun/ch32fun
TARGET_MCU:=CH32V003

ADDITIONAL_C_FILES+=../rv003usb/rv003usb.S
SYSTEM_C:=
EXTRA_CFLAGS:=-I. -I../lib -DUSE_TINY_BOOT -I../rv003usb

LINKER_SCRIPT:=ch32v003fun-usb-bootloader.ld
WRITE_SECTION:=bootloader
FLASH_COMMAND:=$(CH32FUN)/../minichlink/minichlink -a -w $(TARGET).bin $(WRITE_SECTION) -B

include $(CH32FUN)/ch32fun.mk

optionbytes :
	#$(MINICHLINK)/minichlink -w +a55af708 option # Default (But what does it mean??)
	$(MINICHLINK)/minichlink -w +a55aff00 option # Enable bootloader, disable RESET
	#$(MINICHLINK)/minichlink -w +a55aef10 option # Enable bootloader, enable RESET
	#$(MINICHLINK)/minichlink -w +a55adf20 option # Disable bootloader, disable RESET
	#$(MINICHLINK)/minichlink -w +a55acf30 option # Disable bootloader, enable RESET

flash : cv_flash
clean : cv_clean


