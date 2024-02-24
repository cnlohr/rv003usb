#ifndef _USB_CONFIG_H
#define _USB_CONFIG_H

//Defines the number of endpoints for this device. (Always add one for EP0). For two EPs, this should be 3.
#define ENDPOINTS 4

#define USB_PIN_DP 3
#define USB_PIN_DM 4
#define USB_PIN_DPU 5
#define USB_PORT D

#define RV003USB_OPTIMIZE_FLASH 1

#ifndef __ASSEMBLER__

#include <tinyusb_hid.h>

#ifdef INSTANCE_DESCRIPTORS

//Taken from http://www.usbmadesimple.co.uk/ums_ms_desc_dev.htm
static const uint8_t device_descriptor[] = {
	18, //Length
	1,  //Type (Device)
	0x10, 0x01, //Spec
	0xEF, //Device Class
	0x02, //Device Subclass
	0x01, //Device Protocol  (000 = use config descriptor)
	0x08, //Max packet size for EP0 (This has to be 8 because of the USB Low-Speed Standard)
	0x09, 0x12, //ID Vendor
	0x03, 0xc0, //ID Product
	0x10, 0x01, //ID Rev
	1, //Manufacturer string
	2, //Product string
	3, //Serial string
	1, //Max number of configurations
};


static const uint8_t config_descriptor[] = {
	// configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
	9, 					// bLength;
	2,					// bDescriptorType;
	67, 0x00,			// wTotalLength  	

	//https://gist.github.com/tai/acd59b125a007ad47767

	0x02,					// bNumInterfaces (Normally 1)
	0x01,					// bConfigurationValue
	0x04,					// iConfiguration
	0x80,					// bmAttributes (was 0xa0)
	0x64,					// bMaxPower (200mA)

	9,					// bLength
	4,					// bDescriptorType
	0,			// bInterfaceNumber (unused, would normally be used for HID)
	0,					// bAlternateSetting
	1,					// bNumEndpoints
	0x02,					// bInterfaceClass 
	0x02,					// bInterfaceSubClass
	0x01,					// bInterfaceProtocol
	0x05,					// iInterface (For getting the other descriptor)

	0x05, 0x24, 0x00, 0x10, 0x00, // CS_INTERFACE (Release #)
	0x04, 0x24, 0x02, 0x02, // CS_INTERFACE (Capabilities)
	0x05, 0x24, 0x06, 0x0, 0x01, // CS_INTERFACE (Mapping)
	0x05, 0x24, 0x01, 0x0, 0x01, // CS_INTERFACE (Management)

	7,            // endpoint descriptor (For endpoint 1)
	0x05,         // Endpoint Descriptor (Must be 5)
	0x81,         // Endpoint Address
	0x03,         // Attributes
	0x08,	0x00, // Size
	1,            // Interval

	0x09,
	0x04,
	1, 			// interface index
	0,			// altsetting index
	2,			// n endpoints	
	0x0A,		// interface class = CDC-Data
	0x00,		//interface sub-class = unused
	 0x00,		// interface protocol code class = None
	6,			// interface descriptor string index

	7,            // endpoint descriptor (For endpoint 1)
	0x05,         // Endpoint Descriptor (Must be 5)
	0x02,         // Endpoint Address
	0x03,         // Attributes
	0x08,	0x00, // Size
	1,            // Interval

	7,            // endpoint descriptor (For endpoint 1)
	0x05,         // Endpoint Descriptor (Must be 5)
	0x83,         // Endpoint Address
	0x03,         // Attributes
	0x08,	0x00, // Size
	1,            // Interval
};


static const uint8_t test_desc1[] = { 2, 3, 0x01, 0x00 };
static const uint8_t test_desc2[] = { 2, 3, 0x02, 0x00 };

// keyboard example at:
//From http://codeandlife.com/2012/06/18/usb-hid-keyboard-with-v-usb/


//Ever wonder how you have more than 6 keys down at the same time on a USB keyboard?  It's easy. Enumerate two keyboards!

#define STR_MANUFACTURER u""
#define STR_PRODUCT      u""
#define STR_SERIAL       u""

struct usb_string_descriptor_struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wString[];
};
const static struct usb_string_descriptor_struct string0 __attribute__((section(".rodata"))) = {
	4,
	3,
	{0x0409}
};
const static struct usb_string_descriptor_struct string1 __attribute__((section(".rodata")))  = {
	sizeof(STR_MANUFACTURER),
	3,
	STR_MANUFACTURER
};
const static struct usb_string_descriptor_struct string2 __attribute__((section(".rodata")))  = {
	sizeof(STR_PRODUCT),
	3,
	STR_PRODUCT
};
const static struct usb_string_descriptor_struct string3 __attribute__((section(".rodata")))  = {
	sizeof(STR_SERIAL),
	3,
	STR_SERIAL
};



// This table defines which descriptor data is sent for each specific
// request from the host (in wValue and wIndex).
const static struct descriptor_list_struct {
	uint32_t	lIndexValue;
	const uint8_t	*addr;
	uint8_t		length;
} descriptor_list[] = {
	{0x00000100, device_descriptor, sizeof(device_descriptor)},
	{0x00000200, config_descriptor, sizeof(config_descriptor)},
	{0x00000300, (const uint8_t *)&string0, 4},
	{0x04090301, (const uint8_t *)&string1, sizeof(STR_MANUFACTURER)},
	{0x04090302, (const uint8_t *)&string2, sizeof(STR_PRODUCT)},	
	{0x04090303, (const uint8_t *)&string3, sizeof(STR_SERIAL)}
};
#define DESCRIPTOR_LIST_ENTRIES ((sizeof(descriptor_list))/(sizeof(struct descriptor_list_struct)) )

#endif // INSTANCE_DESCRIPTORS

#endif

#endif 
