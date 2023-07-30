#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

// Allow reading and writing to the scratchpad via HID control messages.
uint8_t scratch[255];

int main()
{
	SystemInit();

	usb_setup();

	while(1)
	{
		printf( "%lu %lu %lu %08lx\n", rv003usb_internal_data.delta_se0_cyccount, rv003usb_internal_data.last_se0_cyccount, rv003usb_internal_data.se0_windup, RCC->CTLR );
	}
}

void usb_handle_control_in( struct usb_endpoint * e, uint8_t * sdata, uint32_t sendtok )
{
	int offset = (e->count)<<3;
	int remain = (int)e->max_len - (int)offset;
	if( remain <= 0 )
	{
		usb_send_nak( sendtok );
	}
	else
	{
		if( remain > 8 ) remain = 8;
		usb_send_data( scratch + offset, remain, 0, sendtok );
		// Don't advance, that will be done by ACK packets.
	}
}

void usb_handle_control_out( struct usb_endpoint * e, uint8_t * data, int len )
{
	int offset = e->count<<3;
	int torx = e->max_len - offset;
	if( torx > len ) torx = len;
	if( torx > 0 )
	{
		memcpy( scratch + offset, data, torx );
		e->count++;
	}
}

void usb_handle_control_in_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
	e->count = 0;
	if( reqLen > sizeof( scratch ) ) reqLen = sizeof( scratch );
	e->max_len = reqLen;
}

void usb_handle_control_out_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
	e->count = 0;
	if( reqLen > sizeof( scratch ) ) reqLen = sizeof( scratch );
	e->max_len = reqLen;
}



