#ifndef _USB_CONFIG_H
#define _USB_CONFIG_H

#include "funconfig.h"

//Defines the number of endpoints for this device. (Always add one for EP0). For two EPs, this should be 3.
#define ENDPOINTS 2

#define USB_PORT C     // [A,C,D] GPIO Port to use with D+, D- and DPU
#define USB_PIN_DP 1   // [0-4] GPIO Number for USB D+ Pin
#define USB_PIN_DM 2   // [0-4] GPIO Number for USB D- Pin
//#define USB_PIN_DPU 5  // [0-7] GPIO for feeding the 1.5k Pull-Up on USB D- Pin; Comment out if not used / tied to 3V3!

#define RV003USB_OPTIMIZE_FLASH 0
#define RV003USB_HANDLE_IN_REQUEST 1
#define RV003USB_OTHER_CONTROL 0
#define RV003USB_HANDLE_USER_DATA 0
#define RV003USB_HID_FEATURES 0

#ifndef __ASSEMBLER__

#include <tinyusb_hid.h>
#include <tusb_types.h>

#ifdef INSTANCE_DESCRIPTORS

//Taken from http://www.usbmadesimple.co.uk/ums_ms_desc_dev.htm
static const uint8_t device_descriptor[] = {
    0x12,       // bLength
    0x01,       // bDescriptorType (Device)
    0x10, 0x01,
    0xFF,          // bDeviceClass
    0xFF,          // bDeviceSubClass
    0xFF,          // bDeviceProtocol
    0x08,          // bMaxPacketSize0 64
    0x5E, 0x04, // idVendor 0x045E
    0x8E, 0x02, // idProduct 0x028E
    0x02, 0x00,
    0x01,       // iManufacturer (String Index)
    0x02,       // iProduct (String Index)
    0x03,       // iSerialNumber (String Index)
    0x01,       // bNumConfigurations 1
};

static const uint8_t config_descriptor[] = {
    0x09,        // bLength
    0x02,        // bDescriptorType (Configuration)
    0x99, 0x00,  // wTotalLength 0x99
    0x04,        // bNumInterfaces 4
    0x01,        // bConfigurationValue
    0x00,        // iConfiguration (String Index)
    0xA0,        // bmAttributes (remote wakeup)
    0xFA,        // bMaxPower 500mA

    // Control Interface (0x5D 0xFF)
    0x09,        // bLength
    0x04,        // bDescriptorType (Interface)
    0x00,        // bInterfaceNumber 0
    0x00,        // bAlternateSetting
    0x02,        // bNumEndpoints 2
    0xFF,        // bInterfaceClass
    0x5D,        // bInterfaceSubClass
    0x01,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

    // Gamepad Descriptor
    0x11,        // bLength
    0x21,        // bDescriptorType (HID)
    0x00, 0x01,  // bcdHID 1.10
    0x01,        // SUB_TYPE
    0x25,        // reserved2
    0x81,        // DEVICE_EPADDR_IN
    0x14,        // bMaxDataSizeIn
    0x00, 0x00, 0x00, 0x00, 0x13, // reserved3
    0x02,        // DEVICE_EPADDR_OUT is this right?
    0x14,        // bMaxDataSizeOut
    0x00, 0x00,  // reserved4

    // Report IN Endpoint 1.1
    0x07,        // bLength
    0x05,        // bDescriptorType (Endpoint)
    0x81,        // bEndpointAddress (IN/D2H)
    0x03,        // bmAttributes (Interrupt)
    0x08, 0x00,  // wMaxPacketSize 32
    0x01,        // bInterval 1 (unit depends on device speed)

    // Report OUT Endpoint 1.2
    0x07,        // bLength
    0x05,        // bDescriptorType (Endpoint)
    0x02,        // bEndpointAddress (OUT/H2D)
    0x03,        // bmAttributes (Interrupt)
    0x08, 0x00,  // wMaxPacketSize 32
    0x08,        // bInterval 8 (unit depends on device speed)

    // Interface Audio
    0x09,        // bLength
    0x04,        // bDescriptorType (Interface)
    0x01,        // bInterfaceNumber 1
    0x00,        // bAlternateSetting
    0x04,        // bNumEndpoints 4
    0xFF,        // bInterfaceClass
    0x5D,        // bInterfaceSubClass
    0x03,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

    // Audio Descriptor
    0x1B,        // bLength
    0x21,
    0x00,        
    0x01,
    0x01,
    0x01,
    0x83,        // XINPUT_MIC_IN
    0x40,        // ??
    0x01,        // ??
    0x04,        // XINPUT_AUDIO_OUT
    0x20,        // ??
    0x16,        // ??
    0x85,        // XINPUT_UNK_IN
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x16,
    0x06,        // XINPUT_UNK_OUT
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,

    // Report IN Endpoint 2.1
    0x07,        // bLength
    0x05,        // bDescriptorType (Endpoint)
    0x83,        // bEndpointAddress (XINPUT_MIC_IN)
    0x03,        // bmAttributes (Interrupt)
    0x08, 0x00,  // wMaxPacketSize 32
    0x02,        // bInterval 2 (unit depends on device speed)

    // Report OUT Endpoint 2.2
    0x07,        // bLength
    0x05,        // bDescriptorType (Endpoint)
    0x04,        // bEndpointAddress (XINPUT_AUDIO_OUT)
    0x03,        // bmAttributes (Interrupt)
    0x08, 0x00,  // wMaxPacketSize 32
    0x04,        // bInterval 4 (unit depends on device speed)

    // Report IN Endpoint 2.3
    0x07,        // bLength
    0x05,        // bDescriptorType (Endpoint)
    0x85,        // bEndpointAddress (XINPUT_UNK_IN)
    0x03,        // bmAttributes (Interrupt)
    0x08, 0x00,  // wMaxPacketSize 32
    0x40,        // bInterval 128

    // Report OUT Endpoint 2.4
    0x07,        // bLength
    0x05,        // bDescriptorType (Endpoint)
    0x06,        // bEndpointAddress (XINPUT_UNK_OUT)
    0x03,        // bmAttributes (Interrupt)
    0x08, 0x00,  // wMaxPacketSize 32
    0x10,        // bInterval 16

    // Interface Plugin Module
    0x09,        // bLength
    0x04,        // bDescriptorType (Interface)
    0x02,        // bInterfaceNumber 2
    0x00,        // bAlternateSetting
    0x01,        // bNumEndpoints 1
    0xFF,        // bInterfaceClass
    0x5D,        // bInterfaceSubClass
    0x02,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

    //PluginModuleDescriptor : {
    0x09,        // bLength
    0x21,        // bDescriptorType
    0x00, 0x01,  // version 1.00
    0x01,        // ??
    0x22,        // ??
    0x86,        // XINPUT_PLUGIN_MODULE_IN,
    0x03,        // ??
    0x00,        // ??

    // Report IN Endpoint 3.1
    0x07,        // bLength
    0x05,        // bDescriptorType (Endpoint)
    0x86,        // bEndpointAddress (XINPUT_PLUGIN_MODULE_IN)
    0x03,        // bmAttributes (Interrupt)
    0x08, 0x00,  // wMaxPacketSize 32
    0x10,        // bInterval 8 (unit depends on device speed)

    // Interface Security
    0x09,        // bLength
    0x04,        // bDescriptorType (Interface)
    0x03,        // bInterfaceNumber 3
    0x00,        // bAlternateSetting
    0x00,        // bNumEndpoints 0
    0xFF,        // bInterfaceClass
    0xFD,        // bInterfaceSubClass
    0x13,        // bInterfaceProtocol
    0x04,        // iInterface (String Index)

    // SecurityDescriptor (XSM3)
    0x06,        // bLength
    0x41,        // bDescriptType (Xbox 360)
    0x00,
    0x01,
    0x01,
    0x03,  
};



// keyboard example at:
//From http://codeandlife.com/2012/06/18/usb-hid-keyboard-with-v-usb/


//Ever wonder how you have more than 6 keys down at the same time on a USB keyboard?  It's easy. Enumerate two keyboards!

#define STR_MANUFACTURER u"CNLohr"
#define STR_PRODUCT      u"RV003USB Demo Xinput"
#define STR_SERIAL       u"000"
#define STR_4       	 u"STR 4"

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

const static struct usb_string_descriptor_struct string4 __attribute__((section(".rodata")))  = {
	sizeof(STR_4),
	3,
	STR_4
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
	{0x04090303, (const uint8_t *)&string3, sizeof(STR_SERIAL)},
	{0x04090304, (const uint8_t *)&string4, sizeof(STR_4)}
};
#define DESCRIPTOR_LIST_ENTRIES ((sizeof(descriptor_list))/(sizeof(struct descriptor_list_struct)) )

#endif // INSTANCE_DESCRIPTORS

#endif

#endif 
