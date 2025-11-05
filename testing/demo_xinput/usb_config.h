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
#define RV003USB_OTHER_CONTROL 1
#define RV003USB_HANDLE_USER_DATA 0
#define RV003USB_HID_FEATURES 0

#ifndef __ASSEMBLER__

#ifdef INSTANCE_DESCRIPTORS

//Taken from http://www.usbmadesimple.co.uk/ums_ms_desc_dev.htm
static const uint8_t device_descriptor[] = {
    0x12,       // bLength
    0x01,       // bDescriptorType (Device)
    0x00, 0x02, // bcdUSB (USB Spec 2.0)
    0xFF,       // bDeviceClass (Vendor specific)
    0xFF,       // bDeviceSubClass (Vendor specific)
    0xFF,       // bDeviceProtocol (Vendor specific)
    0x08,       // bMaxPacketSize 8
    0x5E, 0x04, // idVendor 0x045E (Microsoft)
    0x8E, 0x02, // idProduct 0x028E
    0x02, 0x00, // bcd (device release version)
    0x01,       // iManufacturer (String Index)
    0x02,       // iProduct (String Index)
    0x03,       // iSerialNumber (String Index)
    0x01,       // bNumConfigurations 1
};

static const uint8_t config_descriptor[] = {
    0x09,       // bLength
    0x02,       // bDescriptorType (Configuration)
    0x31, 0x00, // wTotalLength
    0x01,       // bNumInterfaces 2
    0x01,       // bConfigurationValue (1 according to spec)
    0x00,       // iConfiguration (String Index)
    0xA0,       // bmAttributes (remote wakeup)
    0xFA,       // bMaxPower 500mA

    // Control Interface (0x5D 0xFF)
    0x09,       // bLength
    0x04,       // bDescriptorType (Interface)
    0x00,       // bInterfaceNumber 0
    0x00,       // bAlternateSetting (XUSB device does not support alternate setting)
    0x02,       // bNumEndpoints 2
    0xFF,       // bInterfaceClass (vendor specified)
    0x5D,       // bInterfaceSubClass (MS USB Devices)
    0x01,       // bInterfaceProtocol (0x1 MS game controller protocol)
    0x00,       // iInterface (String Index)

    // TODO EXPLAIN THESE BETTER
    // Gamepad Descriptor
    0x11,       // bLength
    0x21,       // bDescriptorType (HID)
    0x00, 0x01, // XUSB protocol version (1.00)
    0x01,       // bDeviceSubtype (0x1 for game controller)
    0x25,       // D7…D4: Endpoint type; D3…D0: Number of report supported in this endpoint (Data/status mix endpoint and support 5 reports)
    0x81,       // IN endpoint address
    0x14,       // breportsize (report 0x00)
    0x00,       // breportsize (report 0x01)
    0x00,       // breportsize (report 0x02)
    0x00,       // breportsize (report 0x03)
    0x00,       // breportsize (report 0x04)
    0x13,       // D7…D4: Endpoint type; D3…D0: Number of report supported in this endpoint (Device control endpoint and support 3 reports)
    0x02,       // OUT endpoint address
    0x08,       // breportsize (report 0x00)
    0x00,       // breportsize (report 0x01)
    0x00,       // breportsize (report 0x02)

    // Report IN Endpoint 1.1
    0x07,       // bLength
    0x05,       // bDescriptorType (Endpoint)
    0x81,       // bEndpointAddress (IN/D2H)
    0x03,       // bmAttributes (Interrupt)
    0x08, 0x00, // wMaxPacketSize 32
    0x04,       // bInterval 1 (unit depends on device speed)

    // Report OUT Endpoint 1.2
    0x07,       // bLength
    0x05,       // bDescriptorType (Endpoint)
    0x02,       // bEndpointAddress (OUT/H2D)
    0x03,       // bmAttributes (Interrupt)
    0x08, 0x00, // wMaxPacketSize 32
    0x08,       // bInterval 8 (unit depends on device speed)
};

// static const uint8_t microsoft_os_descriptor[] = {
//  0x12,                                                                   // bLength
//  0x03,                                                                   // bDescriptorType (String)
//  0x4D,0x00,0x53,0x00,0x46,0x00,0x54,0x00,0x31,0x00,0x30,0x00,0x30,0x00,  // "MSFT100"
//  0x69,                                                                   // bMS_VendorCode
//  0x00,                                                                   // bPad
// };

    // 0x28, 0x00, 0x00, 0x00,                                                  // dwLength
    // 0x00, 0x01,                                                              // bcdVersion
    // 0x04, 0x00,                                                              // wIndex
    // 0x01,                                                                    // bCount
    // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                // RESERVED
    // 0x00,                                                                    // bFirstInterfaceNumber
    // 0x01,                                                                    // bNumInterfaces
    // 0x58, 0x55, 0x53, 0x42, 0x31, 0x30, 0x00, 0x00,                          // "XUSB10"
    // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                          // subCompatibleID
    // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                      // RESERVED


// keyboard example at:
//From http://codeandlife.com/2012/06/18/usb-hid-keyboard-with-v-usb/


//Ever wonder how you have more than 6 keys down at the same time on a USB keyboard?  It's easy. Enumerate two keyboards!

#define STR_MANUFACTURER    u"CNLohr"
#define STR_PRODUCT         u"RV003USB Demo Xinput"
#define STR_SERIAL          u"000"
#define STR_4               u"STR 4"

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
    uint32_t        lIndexValue;
    const uint8_t   *addr;
    uint8_t         length;
} descriptor_list[] = {
    {0x00000100, device_descriptor, sizeof(device_descriptor)},
    {0x00000200, config_descriptor, sizeof(config_descriptor)},
    // interface number // 2200 for hid descriptors.
    //{0x00002200, gamepad_hid_desc, sizeof(gamepad_hid_desc)},
    //{0x00002100, config_descriptor + 18, 9 }, // Not sure why, this seems to be useful for Windows + Android.

    {0x00000300, (const uint8_t *)&string0, 4},
    {0x04090301, (const uint8_t *)&string1, sizeof(STR_MANUFACTURER)},
    {0x04090302, (const uint8_t *)&string2, sizeof(STR_PRODUCT)},   
    {0x04090303, (const uint8_t *)&string3, sizeof(STR_SERIAL)},
    {0x04090304, (const uint8_t *)&string4, sizeof(STR_4)},
    // {0x040903EE, microsoft_os_descriptor, sizeof(microsoft_os_descriptor)}
};
#define DESCRIPTOR_LIST_ENTRIES ((sizeof(descriptor_list))/(sizeof(struct descriptor_list_struct)) )

#endif // INSTANCE_DESCRIPTORS

#endif

#endif 