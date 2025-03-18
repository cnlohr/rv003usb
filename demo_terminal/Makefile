TARGET:=demo_terminal
CH32FUN:=../ch32v003fun/ch32fun

ADDITIONAL_C_FILES+=../rv003usb/rv003usb.S ../rv003usb/rv003usb.c
EXTRA_CFLAGS:=-I../lib -I../rv003usb

LDFLAGS+=-Wl,--print-memory-usage

all : $(TARGET).bin

TARGET_MCU?=CH32V003

include $(CH32FUN)/ch32fun.mk

terminal_usb:
	$(MINICHLINK)/minichlink -kT -c 0x1209d003

flash : cv_flash

clean : cv_clean
