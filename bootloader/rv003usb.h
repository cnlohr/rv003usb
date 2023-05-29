#ifndef _RV003USB_H
#define _RV003USB_H

// Port D.3 = D+
// Port D.4 = D-
// Port D.5 = DPU

#define DEBUG_PIN 2
#define USB_DM 3     //DM MUST be BEFORE DP
#define USB_DP 4
#define USB_DPU 5
#define USB_PORT GPIOD
		
#define USB_GPIO_BASE GPIOD_BASE

// Packet Type + 8 + CRC + Buffer
#define USB_BUFFER_SIZE 12

#define USB_DMASK ((1<<(USB_DM)) | 1<<(USB_DP))

#ifndef __ASSEMBLER__

#include "usb_config.h"

#define EMPTY_SEND_BUFFER (uint8_t*)1

// These definitions are from espusb.

struct usb_endpoint
{
	uint8_t count_in;	// ack count
	uint8_t count_out;	// For future: When receiving data.
	uint8_t opaque;     // For user.
	uint8_t toggle_in;   // DATA0 or DATA1?
	uint8_t toggle_out;  //Out PC->US
	uint8_t is_descriptor;
};

struct rv003usb_internal
{
	uint8_t current_endpoint;
	uint8_t my_address; // Will be 0 until set up.
	uint8_t setup_request;
	uint16_t control_max_len;

	// 4 bytes + 4 * ENDPOINTS

	struct usb_endpoint eps[ENDPOINTS];
};

//Detailed analysis of some useful stuff and performance tweaking: http://naberius.de/2015/05/14/esp8266-gpio-output-performance/
//Reverse engineerd boot room can be helpful, too: http://cholla.mmto.org/esp8266/bootrom/boot.txt
//USB Protocol read from wikipedia: https://en.wikipedia.org/wiki/USB
// Neat stuff: http://www.usbmadesimple.co.uk/ums_3.htm
// Neat stuff: http://www.beyondlogic.org/usbnutshell/usb1.shtml

struct usb_urb
{
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint32_t lIndexValue;
	uint16_t wLength;
} __attribute__((packed));


void usb_pid_handle_setup( uint32_t this_token, uint8_t * data );
void usb_pid_handle_in( uint32_t this_token, uint8_t * data, uint32_t last_32_bit, int crc_ok );
void usb_pid_handle_out( uint32_t this_token, uint8_t * data );
void usb_pid_handle_data( uint32_t this_token, uint8_t * data, uint32_t which_data, uint32_t length );
void usb_pid_handle_ack( uint32_t this_token, uint8_t * data );

//poly_function = 0 to include CRC.
//poly_function = 2 to exclude CRC.
//This function is provided in assembly
void usb_send_data( uint8_t * data, uint32_t length, uint32_t poly_function, uint32_t token );
#endif

#endif

