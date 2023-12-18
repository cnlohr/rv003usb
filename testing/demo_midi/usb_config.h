#ifndef _USB_CONFIG_H
#define _USB_CONFIG_H

//Defines the number of endpoints for this device. (Always add one for EP0). For two EPs, this should be 3.
#define ENDPOINTS 3

#define USB_DM 3
#define USB_DP 4
#define USB_DPU 5
#define USB_PORT D

#define RV003USB_EVENT_DEBUGGING 1
#define RV003USB_OPTIMIZE_FLASH 1
#define RV003USB_HANDLE_IN_REQUEST 1
#define RV003USB_OTHER_CONTROL 1
#define RV003USB_HANDLE_USER_DATA 1
#define RV003USB_HID_FEATURES 0

#ifndef __ASSEMBLER__

#include <tinyusb_hid.h>
#include <tusb_types.h>

#ifdef INSTANCE_DESCRIPTORS

//Taken from http://www.usbmadesimple.co.uk/ums_ms_desc_dev.htm
static const uint8_t device_descriptor[] = {
	18, //Length
	TUSB_DESC_DEVICE,  //Type (Device)
	0x10, 0x01, //Spec
	0x0, //Device Class
	0x0, //Device Subclass
	0x0, //Device Protocol  (000 = use config descriptor)
	0x08, //Max packet size for EP0 (This has to be 8 because of the USB Low-Speed Standard)
	0x09, 0x12, //ID Vendor
	0x03, 0xe0, //ID Product
	0x02, 0x00, //ID Rev
	1, //Manufacturer string
	2, //Product string
	3, //Serial string
	1, //Max number of configurations
};

static const uint8_t config_descriptor[] = {
	// configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
	9,                        // bLength;
	TUSB_DESC_CONFIGURATION,  // bDescriptorType;
	101, 0x00,                // wTotalLength
	0x02,                     // bNumInterfaces (Normally 1)
	0x01,                     // bConfigurationValue
	0x00,                     // iConfiguration
	0x80,                     // bmAttributes (was 0xa0)
	0x64,                     // bMaxPower (200mA)

	9,                        // bLength
	TUSB_DESC_INTERFACE,      // bDescriptorType
	0,                        // bInterfaceNumber
	0,                        // bAlternateSetting
	0,                        // bNumEndpoints
	TUSB_CLASS_AUDIO,         // bInterfaceClass (0x03 = HID)
	1,                        // bInterfaceSubClass
	0,                        // bInterfaceProtocol
	0,                        // iInterface

	// B.2.2 Class-specific AC Interface Descriptor
	9,                        // sizeof(usbDescrCDC_HeaderFn): length of descriptor in bytes 
	TUSB_DESC_CS_INTERFACE,   // descriptor type
	1,                        // header functional descriptor
	0x0, 0x01,                // bcdADC
	9, 0,                     // wTotalLength
	1,                        // bInCollection = Number of streaming interfaces.
	1,                        // MIDIStreaming interface 1 belongs to this AudioControl interface.

	// B.3 MIDI Streaming Interface Descriptors
	// B.3.1 Standard MS Interface Descriptor
	9,                        // bLength
	TUSB_DESC_INTERFACE,
	1,                        // Index of this interface.
	0,                        // Index of this alternate setting.
	2,                        // bNumEndpoints = 2
	1,                        // AUDIO
	3,                        // MIDISTREAMING
	0,                        // Unused bInterfaceProtocol
	0,                        // Unused string index for iInterface

	// B.3.2 Class-specific MS Interface Descriptor
	7,                        // bLength
	TUSB_DESC_CS_INTERFACE,   // CS_INTERFACE descriptor
	1,                        // MS_HEADER subtype.
	0x0, 0x01,                // Revision of this class specification
	0x41, 0,                  // Total size of class-specific descriptors

	// B.3.3 MIDI IN Jack Descriptor
	6,                        // bLength
    TUSB_DESC_CS_INTERFACE,   // CS_INTERFACE descriptor
	0x02,                     // bDescriptorSubtype = MIDI_IN_JACK
	0x01,                     // bJackType = EMBEDDED
	0x01,                     // bJackID
	0x00,                     // iJack (unused)

	// B.3.4 MIDI IN Jack Descriptor
	6,                        // bLength
    TUSB_DESC_CS_INTERFACE,   // CS_INTERFACE descriptor
	0x02,                     // bDescriptorSubtype = MIDI_IN_JACK
	0x02,                     // bJackType = EXTERNAL
	0x02,                     // bJackID
	0x00,                     // iJack (unused)

	// B.3.5 MIDI OUT Jack Descriptor
	9,                        // bLength
    TUSB_DESC_CS_INTERFACE,   // CS_INTERFACE descriptor
	0x03,                     // bDescriptorSubtype = MIDI_OUT_JACK
	0x01,                     // bJackType = EMBEDDED
	0x03,                     // bJackID
	0x01,                     // bNrPins = 1
	0x02,                     // BaSourceID
	0x01,                     // BaSourcePin
	0x00,                     // iJack (unused)

	// B.3.4 MIDI OUT Jack Descriptor
	9,                        // bLength
    TUSB_DESC_CS_INTERFACE,   // CS_INTERFACE descriptor
	0x03,                     // bDescriptorSubtype = MIDI_OUT_JACK
	0x02,                     // bJackType = EXTERNAL
	0x04,                     // bJackID
	0x01,                     // bNrPins = 1
	0x01,                     // BaSourceID
	0x01,                     // BaSourcePin
	0x00,                     // iJack (unused)


	// B.4 Bulk OUT Endpoint Descriptors
	// Actually, we are just interrupt.  So yolo.

	//B.4.1 Standard Bulk OUT Endpoint Descriptor
	9,                  // bLength
	TUSB_DESC_ENDPOINT,
	0x1,                // bEndpointAddress
	3,                  // bmAttributes = Interrupt
	8, 0,               // wMaxPacketSize 
	1,                  // bIntervall
	0,                  // bRefresh
	0,                  // bSynchAddress


	// B.4.2 Class-specific MS Bulk OUT Endpoint Descriptor
	5,                  // bLength
	TUSB_DESC_CS_ENDPOINT,
	1,                  // bDescriptorSubtype = MS_GENERAL
	1,                  // bNumEmbMIDIJack = Number of embedded MIDI IN Jacks
	1,                  // BaAssocJackID = 1

	// B.5 Example 1 Bulk IN Endpoint Descriptors
	// B.5.1 Standard Bulk IN Endpoint Descriptor
	9,                  // bLength
	TUSB_DESC_ENDPOINT,
	0x81,               // bEndpointAddress
	3,                  // bmAttributes: 2: Bulk, 3: Interrupt endpoint
	8, 0,               // wMaxPacketSize
	1,                  // bIntervall in ms
	0,                  // bRefresh (unused)
	0,                  // bSyncAddress

	// B.5.2 Class-specific MS Bulk IN Endpoint Descriptor
	5,                  // bLength
	TUSB_DESC_CS_ENDPOINT,
	1,                  // bDescriptorSubtype
	1,                  // bNumEmbMIDIJack
	3,                  // baAssocJackID (0)
};



// keyboard example at:
//From http://codeandlife.com/2012/06/18/usb-hid-keyboard-with-v-usb/


//Ever wonder how you have more than 6 keys down at the same time on a USB keyboard?  It's easy. Enumerate two keyboards!

#define STR_MANUFACTURER u"CNLohr"
#define STR_PRODUCT      u"RV003USB Example MIDI Device"
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
	{0x00000300, (const uint8_t *)&string0, 4},
	{0x04090301, (const uint8_t *)&string1, sizeof(STR_MANUFACTURER)},
	{0x04090302, (const uint8_t *)&string2, sizeof(STR_PRODUCT)},	
	{0x04090303, (const uint8_t *)&string3, sizeof(STR_SERIAL)}
};
#define DESCRIPTOR_LIST_ENTRIES ((sizeof(descriptor_list))/(sizeof(struct descriptor_list_struct)) )

#endif // INSTANCE_DESCRIPTORS

#endif

#endif 
