# Using MCU's serial number as USB serial number

Here is a simple helper utility that reads UUID (ESIG) of the ch32v003 using minichlink and then adds it as a ``STR_SERIAL`` define during build of the firmware. This baking-in process is simpler than reading UUID in code and then formatting it as u16 string for use in USB descriptor.

To use it you need to add these lines to your ``Makefile`` before definitions of make targets (``flash :``, ``clean:`` and others):

```
SERIAL_UTILITY?=../serial_number_utility
MINICHLINK_BUILD:=$(shell make -C $(MINICHLINK) all > /dev/null 2>&1)
SERIAL_BUILD:=$(shell make -C $(SERIAL_UTILITY) all > /dev/null 2>&1)
SERIAL:=$(shell $(SERIAL_UTILITY)/serialnumberutility -m $(MINICHLINK)/minichlink)
EXTRA_CFLAGS+=$(MINICHLINK_BUILD) $(SERIAL_BUILD) -DSTR_SERIAL="u\"$(SERIAL)\""
```

You can check the Makefile for ``demo_hidapi`` as an example.

Then do ``make clean && make``. Connect device to your PC and check the serial number (in linux you can use ``sudo dmesg -w``). If for some reason this utility couldn't read a UUID it will use ``n/a`` as a substitute.

If you don't want to use this method, you can make your own serial number generator and let it pass a serial number to a ``SERIAL`` variable and then use it as ``-DSTR_SERIAL="u\"$(SERIAL)\"`` in ``EXTRA_CFLAGS``.

If no SERIAL is defined via flags, the one defined in usb_config.h will be used instead.
