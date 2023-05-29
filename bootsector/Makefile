all : flash

TARGET:=rv003usb
CH32V003FUN:=../ch32v003fun/ch32v003fun
MINICHLINK?=../ch32v003fun/minichlink

ADDITIONAL_C_FILES+=rv003usb.S
SYSTEM_C:=

include ../ch32v003fun/ch32v003fun/ch32v003fun.mk

flash : cv_flash
clean : cv_clean


