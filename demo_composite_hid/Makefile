all : flash

TARGET:=demo_composite_hid
CH32FUN:=../ch32v003fun/ch32fun
TARGET_MCU:=CH32V003

ADDITIONAL_C_FILES+=../rv003usb/rv003usb.S ../rv003usb/rv003usb.c
EXTRA_CFLAGS:=-I../lib -I../rv003usb

include $(CH32FUN)/ch32fun.mk

flash : cv_flash
clean : cv_clean


