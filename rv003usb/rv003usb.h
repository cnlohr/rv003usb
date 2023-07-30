#ifndef _RV003USB_H
#define _RV003USB_H

// Port D.3 = D+
// Port D.4 = D-
// Port D.5 = DPU

#define USB_GPIO_BASE GPIOD_BASE

// Packet Type + 8 + CRC + Buffer
#define USB_BUFFER_SIZE 12

#define USB_DMASK ((1<<(USB_DM)) | 1<<(USB_DP))

#include "usb_config.h"


#ifdef  REALLY_TINY_COMP_FLASH
#define MY_ADDRESS_OFFSET_BYTES 4
#define LAST_SE0_OFFSET         12
#define DELTA_SE0_OFFSET        16
#define SE0_WINDUP_OFFSET       20
#define ENDP_OFFSET             28
#define SETUP_REQUEST_OFFSET    24
#else
#define MY_ADDRESS_OFFSET_BYTES 1
#define LAST_SE0_OFFSET         4
#define DELTA_SE0_OFFSET        8
#define SE0_WINDUP_OFFSET       12
#define ENDP_OFFSET             17
#define SETUP_REQUEST_OFFSET    16
#endif

#if USB_IN_DATA_SECTION
#define HANDLER_SECTION .srodata
#define HANDLE_ATTRIBUTES __attribute__((section(".srodata")))
#else
#define HANDLE_ATTRIBUTES __attribute__((section(".text")))
#define HANDLER_SECTION .text.vector_handler
#endif

#ifndef __ASSEMBLER__

#define EMPTY_SEND_BUFFER (uint8_t*)1

// This can be undone once support for fast-c.lbu / c.sbu is made available.
#ifdef  REALLY_TINY_COMP_FLASH
#define TURBO8TYPE uint32_t
#define TURBO16TYPE uint32_t
#else
#define TURBO8TYPE uint8_t
#define TURBO16TYPE uint16_t
#endif


struct usb_endpoint
{
	TURBO8TYPE count_in;	// ack count
	TURBO8TYPE count_out;	// For future: When receiving data.
	TURBO8TYPE opaque;     // For user.
	TURBO8TYPE toggle_in;   // DATA0 or DATA1?
	TURBO8TYPE toggle_out;  //Out PC->US
	TURBO8TYPE is_descriptor;
#ifdef REALLY_TINY_COMP_FLASH
	TURBO8TYPE reserved1, reserved2;
#endif
};


struct rv003usb_internal
{
	TURBO8TYPE current_endpoint;
	TURBO8TYPE my_address; // Will be 0 until set up.
	TURBO16TYPE control_max_len;
	uint32_t last_se0_cyccount;
	int32_t delta_se0_cyccount;
	uint32_t se0_windup;
	uint32_t setup_request;  //XXX TODO: Roll this into current_endpoint, as the MSB.
	struct usb_endpoint eps[ENDPOINTS];
};

//Detailed analysis of some useful stuff and performance tweaking: http://naberius.de/2015/05/14/esp8266-gpio-output-performance/
//Reverse engineerd boot room can be helpful, too: http://cholla.mmto.org/esp8266/bootrom/boot.txt
//USB Protocol read from wikipedia: https://en.wikipedia.org/wiki/USB
// Neat stuff: http://www.usbmadesimple.co.uk/ums_3.htm
// Neat stuff: http://www.beyondlogic.org/usbnutshell/usb1.shtml

struct usb_urb
{
	uint16_t wRequestTypeLSBRequestMSB;
	uint32_t lValueLSBIndexMSB;
	uint16_t wLength;
};


// Note: This checks addr & endp to make sure they are valid.
void usb_pid_handle_setup( uint32_t addr, uint8_t * data, uint32_t endp, uint32_t unused, struct rv003usb_internal * ist ) HANDLE_ATTRIBUTES;
void usb_pid_handle_in( uint32_t addr, uint8_t * data, uint32_t endp, uint32_t unused, struct rv003usb_internal * ist )    HANDLE_ATTRIBUTES;
void usb_pid_handle_out( uint32_t addr, uint8_t * data, uint32_t endp, uint32_t unused, struct rv003usb_internal * ist )   HANDLE_ATTRIBUTES;

void usb_pid_handle_data( uint32_t this_token, uint8_t * data, uint32_t which_data, uint32_t length, struct rv003usb_internal * ist ) HANDLE_ATTRIBUTES;
void usb_pid_handle_ack( uint32_t dummy, uint8_t * data, uint32_t dummy2, uint32_t dummy3, struct rv003usb_internal * ist  ) HANDLE_ATTRIBUTES;

//poly_function = 0 to include CRC.
//poly_function = 2 to exclude CRC.
//This function is provided in assembly
void usb_send_data( uint8_t * data, uint32_t length, uint32_t poly_function, uint32_t token );
#endif

#endif

