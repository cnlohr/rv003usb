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

#ifndef __ASSEMBLER__

#define EMPTY_SEND_BUFFER (uint8_t*)1

// This can be undone once support for fast-c.lbu / c.sbu is made available.
#ifdef  REALLY_TINY_COMP_FLASH
#define TURBO8TYPE uint32_t
#else
#define TURBO8TYPE uint8_t
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
	uint8_t current_endpoint;
	uint8_t my_address; // Will be 0 until set up.
	uint16_t control_max_len;
	uint32_t last_se0_cyccount;
	int32_t delta_se0_cyccount;
	uint32_t se0_windup;
	uint8_t setup_request;
	// 5 bytes + 6 * ENDPOINTS
#ifdef REALLY_TINY_COMP_FLASH
	uint8_t reserved1, reserved2, reserved3;
#endif

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
} __attribute__((packed));


// Note: This checks addr & endp to make sure they are valid.
void usb_pid_handle_setup( uint32_t addr, uint8_t * data, uint32_t endp, uint32_t unused, struct rv003usb_internal * ist );
void usb_pid_handle_in( uint32_t addr, uint8_t * data, uint32_t endp, uint32_t unused, struct rv003usb_internal * ist );
void usb_pid_handle_out( uint32_t addr, uint8_t * data, uint32_t endp, uint32_t unused, struct rv003usb_internal * ist );

void usb_pid_handle_data( uint32_t this_token, uint8_t * data, uint32_t which_data, uint32_t length, struct rv003usb_internal * ist );
void usb_pid_handle_ack( uint32_t dummy, uint8_t * data, uint32_t dummy2, uint32_t dummy3, struct rv003usb_internal * ist  );

//poly_function = 0 to include CRC.
//poly_function = 2 to exclude CRC.
//This function is provided in assembly
void usb_send_data( uint8_t * data, uint32_t length, uint32_t poly_function, uint32_t token );
#endif

#endif

