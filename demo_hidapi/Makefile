all : flash

TARGET:=demo_hidapi
CH32FUN:=../ch32fun/ch32fun
TARGET_MCU:=CH32V003
SERIAL_UTILITY?=../serial_number_utility
ADDITIONAL_C_FILES+=../rv003usb/rv003usb.S ../rv003usb/rv003usb.c

include $(CH32FUN)/ch32fun.mk
MINICHLINK_BUILD:=$(shell make -C $(MINICHLINK) all > /dev/null 2>&1)
SERIAL_BUILD:=$(shell make -C $(SERIAL_UTILITY) all > /dev/null 2>&1)
SERIAL:=$(shell $(SERIAL_UTILITY)/serialnumberutility -m $(MINICHLINK)/minichlink)
EXTRA_CFLAGS:=$(MINICHLINK_BUILD) $(SERIAL_BUILD) -I../lib -I../rv003usb -DSTR_SERIAL="u\"$(SERIAL)\""

cv_flash : $(TARGET).bin
	$(FLASH_COMMAND)

flash : cv_flash
clean : cv_clean


