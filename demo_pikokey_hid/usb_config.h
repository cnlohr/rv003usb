#ifndef _USB_CONFIG_H
#define _USB_CONFIG_H

//Defines the number of endpoints for this device. (Always add one for EP0). For two EPs, this should be 3.
#define ENDPOINTS 2

#define USB_PORT D     // [A,C,D] GPIO Port to use with D+, D- and DPU
#define USB_PIN_DP 3   // [0-4] GPIO Number for USB D+ Pin
#define USB_PIN_DM 4   // [0-4] GPIO Number for USB D- Pin
// #define USB_PIN_DPU 5  // [0-7] GPIO for feeding the 1.5k Pull-Up on USB D- Pin; Comment out if not used / tied to 3V3!

#define RV003USB_OPTIMIZE_FLASH 1
#define RV003USB_EVENT_DEBUGGING 0
#define RV003USB_HANDLE_IN_REQUEST 1
#define RV003USB_OTHER_CONTROL 0
#define RV003USB_HANDLE_USER_DATA 0
#define RV003USB_HID_FEATURES 0

#ifndef __ASSEMBLER__

#include <tinyusb_hid.h>

#ifdef INSTANCE_DESCRIPTORS
//Taken from http://www.usbmadesimple.co.uk/ums_ms_desc_dev.htm
static const uint8_t device_descriptor[] = {
	18, //Length
	1,  //Type (Device)
	0x10, 0x01, //Spec
	0x0, //Device Class
	0x0, //Device Subclass
	0x0, //Device Protocol  (000 = use config descriptor)
	0x08, //Max packet size for EP0 (This has to be 8 because of the USB Low-Speed Standard)
	0xD0, 0x16, //ID Vendor
	0xB9, 0x13, //ID Product
	0x02, 0x00, //ID Rev
	1, //Manufacturer string
	2, //Product string
	3, //Serial string
	1, //Max number of configurations
};

// static const uint8_t mouse_hid_desc[] = {  //From http://eleccelerator.com/tutorial-about-usb-hid-report-descriptors/
// 	HID_USAGE_PAGE( HID_USAGE_PAGE_DESKTOP ),         // USAGE_PAGE (Generic Desktop)
// 	HID_USAGE( HID_USAGE_DESKTOP_MOUSE ),  // USAGE (Mouse)
// 	HID_COLLECTION ( HID_COLLECTION_APPLICATION ),    // COLLECTION (Application)
// 		HID_USAGE( HID_USAGE_DESKTOP_POINTER ),       //   USAGE (Pointer)
// 		HID_COLLECTION ( HID_COLLECTION_PHYSICAL ),   //   COLLECTION (Physical)
// 			HID_USAGE_PAGE( HID_USAGE_PAGE_BUTTON ),  //     USAGE_PAGE (Button)
// 			HID_USAGE_MIN( 1 ),                       //     USAGE_MINIMUM (Button 1)
// 			HID_USAGE_MAX( 3 ),                       //     USAGE_MAXIMUM (Button 3)
// 			HID_LOGICAL_MIN( 0 ),                     //     LOGICAL_MINIMUM (0)
// 			HID_LOGICAL_MAX( 1 ),                     //     LOGICAL_MAXIMUM (1)
// 			HID_REPORT_COUNT( 3 ),                    //     REPORT_COUNT (3)
// 			HID_REPORT_SIZE( 1 ),                     //     REPORT_SIZE (1)
// 			HID_INPUT( 0x02 ),                        //     INPUT (Data,Var,Abs)
// 			HID_REPORT_COUNT( 1 ),                    //     REPORT_COUNT (1)
// 			HID_REPORT_SIZE( 5 ),                     //     REPORT_SIZE (5)
// 			HID_INPUT( 0x03 ),                        //     INPUT (Cnst,Var,Abs)
// 			HID_USAGE_PAGE( HID_USAGE_PAGE_DESKTOP ), //     USAGE_PAGE (Desktop)
// 			HID_USAGE( HID_USAGE_DESKTOP_X ),         //     USAGE (X)
// 			HID_USAGE( HID_USAGE_DESKTOP_Y ),         //     USAGE (Y)
// 			HID_USAGE( HID_USAGE_DESKTOP_WHEEL ),     //     USAGE (Wheel)
// 			HID_LOGICAL_MIN( 0x81 ),                  //     LOGICAL_MINIMUM -127
// 			HID_LOGICAL_MAX( 0x7f ),                  //     LOGICAL_MAXIMUM (127)
// 			HID_REPORT_SIZE( 8 ),                     //     REPORT_SIZE (8)
// 			HID_REPORT_COUNT( 3 ),                    //     REPORT_COUNT (3)
// 			HID_INPUT( 0x06 ),                        //     INPUT (Data,Var,Rel)
// 		HID_COLLECTION_END,                           //   END_COLLECTION
// 	HID_COLLECTION_END,                               // END_COLLECTIONs

// 	// Tack this on to do custom HID commands.
// 	/*
// 	HID_COLLECTION ( HID_COLLECTION_APPLICATION )                 ,
// 		HID_REPORT_ID    ( 0xaa                                   )
// 		HID_USAGE        ( 0xff              ) ,
// 		HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE    ) ,
// 		HID_REPORT_COUNT ( 8 ) ,
// 	HID_COLLECTION_END,
// 	*/
// };

//From http://codeandlife.com/2012/06/18/usb-hid-keyboard-with-v-usb/
static const uint8_t keyboard_hid_desc[] = {   /* USB report descriptor */
	HID_USAGE_PAGE( HID_USAGE_PAGE_DESKTOP ),         // USAGE_PAGE (Generic Desktop)
	HID_USAGE( HID_USAGE_DESKTOP_KEYBOARD ),          // USAGE (Keyboard)
	HID_COLLECTION ( HID_COLLECTION_APPLICATION ),    // COLLECTION (Application)
		HID_REPORT_SIZE( 1 ),                         //     REPORT_SIZE (8)
		HID_REPORT_COUNT( 8 ),                        //     REPORT_COUNT (3)
		HID_USAGE_PAGE( HID_USAGE_PAGE_KEYBOARD ),    //     USAGE_PAGE (Keyboard)(Key Codes)
    	HID_USAGE_MIN( 0xe0 ),                        //     USAGE_MINIMUM (Keyboard LeftControl)(224)
    	HID_USAGE_MAX( 0xe7 ),                        //     USAGE_MAXIMUM (Keyboard Right GUI)(231)
		HID_LOGICAL_MIN( 0 ),                         //     LOGICAL_MINIMUM (0)
		HID_LOGICAL_MAX( 1 ),                         //     LOGICAL_MAXIMUM (1)
		HID_INPUT( 0x02 ),                            //     INPUT (Data,Var,Abs) ; Modifier byte
		HID_REPORT_COUNT( 1 ),                        //     REPORT_COUNT (1)
		HID_REPORT_SIZE( 8 ),                         //     REPORT_SIZE (8)
		HID_INPUT( 0x03 ),                            //     INPUT (Cnst,Var,Abs) ; Reserved byte
		HID_REPORT_COUNT( 5 ),                        //     REPORT_COUNT (5)
		HID_REPORT_SIZE( 1 ),                         //     REPORT_SIZE (1)
		HID_USAGE_PAGE( HID_USAGE_PAGE_LED ),         //     USAGE_PAGE (LEDs)
    	HID_USAGE_MIN( 0x01 ),                        //     USAGE_MINIMUM (Num Lock)
	    HID_USAGE_MAX( 0x05 ),                        //     USAGE_MAXIMUM (Kana)
		HID_OUTPUT( 0x02 ),                           //     OUTPUT (Data,Var,Abs) ; LED report
		HID_REPORT_COUNT( 1 ),                        //     REPORT_COUNT (1)
		HID_REPORT_SIZE( 3 ),                         //     REPORT_SIZE (3)
		HID_OUTPUT( 0x03 ),                           //     OUTPUT (Cnst,Var,Abs) ; LED report padding
		HID_REPORT_COUNT( 6 ),                        //     REPORT_COUNT (6)
		HID_REPORT_SIZE( 8 ),                         //     REPORT_SIZE (8)
		HID_LOGICAL_MIN( 0 ),                         //     LOGICAL_MINIMUM (0)
		HID_LOGICAL_MAX( 167 ),                       //     LOGICAL_MAXIMUM (167)  (Normally would be 101, but we want volume buttons)
    	HID_USAGE_PAGE( HID_USAGE_PAGE_KEYBOARD ),     //     USAGE_PAGE (Keyboard)(Key Codes)
    	HID_USAGE_MIN( 0x00 ),                        //     USAGE_MINIMUM (0)
	    HID_USAGE_MAX( 167 ),                         //     USAGE_MAXIMUM (Keyboard Application)(101) (Now 167)
	HID_INPUT( 0 ),                                   //   INPUT (Data,Ary,Abs)
    HID_COLLECTION_END,                               // END_COLLECTION
};

//Ever wonder how you have more than 6 keys down at the same time on a USB keyboard?  It's easy. Enumerate two keyboards!
// No, really, that's what some hardware manufacturers do.

static const uint8_t config_descriptor[] = {  //Mostly stolen from a USB mouse I found.
	// configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
	9, 					// bLength;
	2,					// bDescriptorType;
	0x22, 0x00,			// wTotalLength 
	0x01,					// bNumInterfaces (Normally 1)
	0x01,					// bConfigurationValue
	0x00,					// iConfiguration
	0x80,					// bmAttributes (was 0xa0)
	0x64,					// bMaxPower (200mA)

	// // This descriptor shows how to embed two HIDs, to build a composite HID device.


	// //Mouse
	// 9,					// bLength
	// 4,					// bDescriptorType
	// 0,			// bInterfaceNumber (unused, would normally be used for HID)
	// 0,					// bAlternateSetting
	// 1,					// bNumEndpoints
	// 0x03,					// bInterfaceClass (0x03 = HID)
	// 0x01,					// bInterfaceSubClass
	// 0x02,					// bInterfaceProtocol (Mouse)
	// 0,					// iInterface

	// 9,					// bLength
	// 0x21,					// bDescriptorType (HID)
	// 0x10,0x01,		//bcd 1.1
	// 0x00, //country code
	// 0x01, //Num descriptors
	// 0x22, //DescriptorType[0] (HID)
	// sizeof(mouse_hid_desc), 0x00,

	// 7, //endpoint descriptor (For endpoint 1)
	// 0x05, //Endpoint Descriptor (Must be 5)
	// 0x81, //Endpoint Address
	// 0x03, //Attributes
	// 0x04,	0x00, //Size
	// 10, //Interval (Number of milliseconds between polls)


	//Keyboard  (It is unusual that this would be here)
	9,					// bLength
	4,					// bDescriptorType
	0,			// bInterfaceNumber  = 1 instead of 0 -- well make it second.
	0,					// bAlternateSetting
	1,					// bNumEndpoints
	0x03,					// bInterfaceClass (0x03 = HID)
	0x01,					// bInterfaceSubClass
	0x01,					// bInterfaceProtocol (??)
	0,					// iInterface

	9,					// bLength
	0x21,					// bDescriptorType (HID)
	0x10,0x01,		//bcd 1.1
	0x00, //country code
	0x01, //Num descriptors
	0x22, //DescriptorType[0] (HID)
	sizeof(keyboard_hid_desc), 0x00, 

	7, //endpoint descriptor (For endpoint 1)
	0x05, //Endpoint Descriptor (Must be 5)
	0x81, //Endpoint Address
	0x03, //Attributes
	0x08,	0x00, //Size (8 bytes)
	10, //Interval Number of milliseconds between polls.
};

#define STR_MANUFACTURER u"Prokyber"
#define STR_PRODUCT      u"PikoKey"
#define STR_SERIAL       u"000"

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
	// {0x00002200, mouse_hid_desc, sizeof(mouse_hid_desc)},
	{0x00002200, keyboard_hid_desc, sizeof(keyboard_hid_desc)},
	{0x00000300, (const uint8_t *)&string0, 4},
	{0x04090301, (const uint8_t *)&string1, sizeof(STR_MANUFACTURER)},
	{0x04090302, (const uint8_t *)&string2, sizeof(STR_PRODUCT)},	
	{0x04090303, (const uint8_t *)&string3, sizeof(STR_SERIAL)}
};
#define DESCRIPTOR_LIST_ENTRIES ((sizeof(descriptor_list))/(sizeof(struct descriptor_list_struct)) )

#endif // INSTANCE_DESCRIPTORS

#endif

#endif 
