#ifndef _USB_CONFIG_H
#define _USB_CONFIG_H

//Defines the number of endpoints for this device. (Always add one for EP0). For two EPs, this should be 3.
#define ENDPOINTS 4

#define USB_PIN_DP 3
#define USB_PIN_DM 4
#define USB_PIN_DPU 5
#define USB_PORT D

#define RV003USB_HANDLE_IN_REQUEST 1
#define RV003USB_OTHER_CONTROL     1
#define RV003USB_HANDLE_USER_DATA  1
#define RV003USB_HID_FEATURES      0
#define RV003USB_EVENT_DEBUGGING   1

#ifndef __ASSEMBLER__

#include <tusb_types.h>
#include <cdc.h>

#ifdef INSTANCE_DESCRIPTORS

//Taken from http://www.usbmadesimple.co.uk/ums_ms_desc_dev.htm
static const uint8_t device_descriptor[] = {
	18, //Length
	TUSB_DESC_DEVICE,  //Type (Device)
	0x10, 0x01, //Spec
	TUSB_CLASS_APPLICATION_SPECIFIC, //Device Class (Let config decide)
	MISC_SUBCLASS_COMMON, //Subclass
	0x00, //Device Protocol
//	TUSB_CLASS_CDC, MISC_SUBCLASS_COMMON, 0,  // Appears that you can go either way... app specific (use config descriptor) or this.
	0x08, //Max packet size for EP0 (This has to be 8 because of the USB Low-Speed Standard)
	0x09, 0x12, //ID Vendor
	0x03, 0xc3, //ID Product
	0x10, 0x01, //ID Rev
	1, //Manufacturer string
	2, //Product string
	3, //Serial string
	1, //Max number of configurations
};

static const uint8_t config_descriptor[] = {
	// configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
	// based on https://gist.github.com/tai/acd59b125a007ad47767

	9,                        // bLength;
	TUSB_DESC_CONFIGURATION,  // bDescriptorType;
	67, 0x00,                 // wTotalLength  	
	0x02,                     // bNumInterfaces (Normally 1)
	0x01,                     // bConfigurationValue
	0x00,                     // iConfiguration
	0x80,                     // bmAttributes (was 0xa0)
	0x64,                     // bMaxPower (200mA)

	9,                        // bLength
	TUSB_DESC_INTERFACE,      // bDescriptorType
	0,                        // bInterfaceNumber (unused, would normally be used for HID)
	0,                        // bAlternateSetting
	1,                        // bNumEndpoints
	TUSB_CLASS_CDC,           // bInterfaceClass    (CDC)
	CDC_COMM_SUBCLASS_ABSTRACT_CONTROL_MODEL,  // bInterfaceSubClass (ABSTRACT CONTROL MODEL)
	CDC_COMM_PROTOCOL_ATCOMMAND,               // bInterfaceProtocol (V25TER_PROTOCOL)
	0x00,                     // iInterface (For getting the other descriptor)

	0x05,
	TUSB_DESC_CS_INTERFACE,
	CDC_FUNC_DESC_HEADER,
	0x10, 0x01,               // CS_INTERFACE (Release #) (CDC_FUNC_DESCR_HEADER)

	0x04,
	TUSB_DESC_CS_INTERFACE,
	CDC_FUNC_DESC_ABSTRACT_CONTROL_MANAGEMENT,
	0x00, // CS_INTERFACE (Capabilities) (CDC_FUNC_DESCR_ABSTRACT_CTRL_MGMNT) -> Contiki says 0, tinyusb says 0, sparetimelabs says

	0x05,
	TUSB_DESC_CS_INTERFACE,
	CDC_FUNC_DESC_UNION,
	0x00, 0x01, // CS_INTERFACE (Mapping) (CDC_FUNC_DESCR_UNION)

	0x05,
	TUSB_DESC_CS_INTERFACE,
	CDC_FUNC_DESC_CALL_MANAGEMENT,
	0x02, 0x01, // CS_INTERFACE (Management) (CDC_FUNC_DESCR_CALL_MGMNT)

	7,                    // endpoint descriptor (For endpoint 1)
	TUSB_DESC_ENDPOINT,   // Endpoint Descriptor (Must be 5)
	0x81,                 // Endpoint Address
	0x03,                 // Attributes
	0x08,	0x00,         // Size
	1,                    // Interval

	0x09,
	0x04,
	1, 			// interface index
	0,			// altsetting index
	2,			// n endpoints	
	TUSB_CLASS_CDC_DATA,	// interface class = CDC-Data
	0x00,		//interface sub-class = unused (SHOULD ALWAYS BE ZERO BY SPEC)
	CDC_DATA_PROTOCOL_TRANSPARENT,		// interface protocol code class = None  <<<<<<<<< THIS IS MEGA SUS.
	0,			// interface descriptor string index

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

#define STR_MANUFACTURER u"cnlohr"
#define STR_PRODUCT      u"CDC Tester"
#define STR_SERIAL       u"0000"

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
