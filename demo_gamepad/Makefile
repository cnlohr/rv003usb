all : flash

TARGET:=demo_gamepad

ADDITIONAL_C_FILES+=../rv003usb/rv003usb.S ../rv003usb/rv003usb.c
EXTRA_CFLAGS:=-I../lib -I../rv003usb

TARGET_MCU?=CH32V003
include ../ch32v003fun/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean


