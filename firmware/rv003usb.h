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

#define USB_BUFFER_SIZE 16

#define USB_DMASK ((1<<(USB_DM)) | 1<<(USB_DP))

#ifndef __ASSEMBLER__

#include "usb_config.h"

#define EMPTY_SEND_BUFFER (uint8_t*)1

// These definitions are from espusb.

struct usb_endpoint
{
	const uint8_t * ptr_in;		// Pointer to "IN" data (US->PC)
	uint16_t size_in;		// Total size of the structure pointed to by ptr_in
	uint16_t advance_in;		// How much data was sent this packet? (How much to advance in ack)
	uint16_t place_in;		// Where in the ptr_in we are currently pointing.
	uint8_t toggle_in; 		// DATA0 or DATA1?
	uint8_t send;			// Sets back to 0 when done sending.
	int * transfer_in_done_ptr;

	uint8_t * ptr_out;
	int * transfer_done_ptr;  //Written to # of bytes received when a datagram is done.
	uint16_t max_size_out;
	uint16_t got_size_out;
	uint8_t toggle_out;  //Out PC->US
};

struct rv003usb_internal
{
	uint8_t usb_buffer[USB_BUFFER_SIZE];
	uint8_t setup_request;
	uint8_t my_address;
	uint8_t there_is_a_host;
	uint8_t packet_size; //Of data currently in usb_buffer

	struct usb_endpoint * ce;  //Current endpoint (set by IN/OUT PIDs)
	struct usb_endpoint eps[ENDPOINTS];
};

//Detailed analysis of some useful stuff and performance tweaking: http://naberius.de/2015/05/14/esp8266-gpio-output-performance/
//Reverse engineerd boot room can be helpful, too: http://cholla.mmto.org/esp8266/bootrom/boot.txt
//USB Protocol read from wikipedia: https://en.wikipedia.org/wiki/USB
// Neat stuff: http://www.usbmadesimple.co.uk/ums_3.htm
// Neat stuff: http://www.beyondlogic.org/usbnutshell/usb1.shtml

struct usb_urb
{
	uint8_t pktp;
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
} __attribute__((packed));


void usb_pid_handle_setup( uint32_t this_token, struct rv003usb_internal * ist );
void usb_pid_handle_in( uint32_t this_token, struct rv003usb_internal * ist, uint32_t last_32_bit, int crc_ok );
void usb_pid_handle_out( uint32_t this_token, struct rv003usb_internal * ist );
void usb_pid_handle_data( uint32_t this_token, struct rv003usb_internal * ist, uint32_t which_data );
void usb_pid_handle_ack( uint32_t this_token, struct rv003usb_internal * ist );

//poly_function = 0 to include CRC.
//poly_function = 2 to exclude CRC.
//This function is provided in assembly
void usb_send_data( uint8_t * data, uint32_t length, uint32_t poly_function );
#endif

#endif

