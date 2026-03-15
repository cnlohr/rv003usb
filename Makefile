
PROJECTS:=bootloader demo_composite_hid demo_hidapi demo_gamepad demo_pikokey_hid testing/cdc_exp testing/demo_midi testing/sandbox testing/test_ethernet testing/demo_xinput

all : build

build :
	for dir in $(PROJECTS); do $(MAKE) -C $$dir build || exit 1; done

clean : $(PROJECTS)
	for dir in $(PROJECTS); do $(MAKE) -C $$dir clean; done

.PHONY : $(PROJECTS)

