all : flash

#PREFIX:=/opt/rv32ec/bin/riscv32-unknown-elf

TARGET:=rv003usb
CH32V003FUN:=../ch32v003fun/ch32v003fun
MINICHLINK?=../ch32v003fun/minichlink

ADDITIONAL_C_FILES+=../rv003usb/rv003usb.S
EXTRA_CFLAGS:=-I../lib -I../rv003usb

include ../ch32v003fun/ch32v003fun/ch32v003fun.mk

flash : cv_flash
clean : cv_clean


