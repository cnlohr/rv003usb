#ifndef PROGRAMMER_CONFIG_H
#define PROGRAMMER_CONFIG_H

#define DEBUG_PRINT              0

#define CH5xx_SUPPORT            1
#define CH5xx_EEPROM             1 
#define CH5xx_OPTIONS            1
#define CH5xx_UNLOCK             1

#define UPDI_SUPPORT             0
#define USB_HID_INTERFACE        1
#define USB_VENDOR_INTERFACE     0
#define WEBSOCKET_INTERFACE      0
#define WEB_INTERFACE            0
#define BLE_INTERFACE            0
#define MCO_SUPPORT              0

#define PROGRAMMER_TYPE_UNKNOWN  0
#define PROGRAMMER_TYPE_ESP32S2  1
#define PROGRAMMER_TYPE_CH32V003 2
#define PROGRAMMER_TYPE_CH570    3
#define PROGRAMMER_TYPE_ESP32    4
#define PROGRAMMER_TYPE_SWADGE   5

#define PROGRAMMER_TYPE          PROGRAMMER_TYPE_CH32V003

#define HAS_MCO                  (MCO_SUPPORT << 12)
#define HAS_BLE                  (BLE_INTERFACE << 11)
#define HAS_WEB_UI               (WEB_INTERFACE << 10)
#define HAS_WEBSOCKET            (WEBSOCKET_INTERFACE << 9)
#define HAS_USB_VENDOR           (USB_VENDOR_INTERFACE << 8)
#define HAS_USB_HID              (USB_HID_INTERFACE << 7)
#define HAS_UPDI_SUPPORT         (UPDI_SUPPORT << 5)
#define HAS_CH5xx_UNLOCK         (CH5xx_UNLOCK << 3)
#define HAS_CH5xx_OPTIONS        (CH5xx_OPTIONS << 2)
#define HAS_CH5xx_EEPROM         (CH5xx_EEPROM << 1)
#define HAS_CH5xx_SUPPORT        (CH5xx_SUPPORT << 0)

#define CAPABILITIES HAS_CH5xx_SUPPORT | \
                     HAS_CH5xx_EEPROM  | \
                     HAS_CH5xx_OPTIONS | \
                     HAS_CH5xx_UNLOCK  | \
                     HAS_UPDI_SUPPORT  | \
                     HAS_USB_HID

#define MAX_IN_TIMEOUT           1000

// This is a hacky thing, but if you are laaaaazzzyyyy and don't want to add a 10k
// resistor, youcan do this.  It glitches the line high very, very briefly.  
// Enable for when you don't have a 10k pull-upand are relying on the internal pull-up.
// WARNING: If you set this, you should set the drive current to 5mA.  This is 
// only useful / needed if you are using the built-in functions.
#define R_GLITCH_HIGH

#endif