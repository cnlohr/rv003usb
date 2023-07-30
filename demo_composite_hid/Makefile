all : flash

TARGET:=demo_composite_hid
CH32V003FUN:=../ch32v003fun/ch32v003fun
MINICHLINK?=../ch32v003fun/minichlink

ADDITIONAL_C_FILES+=../rv003usb/rv003usb.S ../rv003usb/rv003usb.c
EXTRA_CFLAGS:=-I../lib -I../rv003usb

include ../ch32v003fun/ch32v003fun/ch32v003fun.mk

flash : cv_flash
clean : cv_clean


