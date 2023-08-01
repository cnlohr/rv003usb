#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

#define WS2812DMA_IMPLEMENTATION
#include "ws2812b_dma_spi_led_driver.h"

// Allow reading and writing to the scratchpad via HID control messages.
uint8_t scratch[255];

uint32_t WS2812BLEDCallback( int ledno )
{
	uint8_t * k = &scratch[ledno * 3];
	return k[0] | (k[1]<<8) | (k[2]<<16);
}


#define NUMUEVENTS 32
uint32_t events[4*NUMUEVENTS];
volatile uint8_t eventhead, eventtail;
void LogUEvent( uint32_t a, uint32_t b, uint32_t c, uint32_t d )
{
	int event = (eventhead + 1) & (NUMUEVENTS-1);
	uint32_t * e = &events[event*4];
	e[0] = a;
	e[1] = b;
	e[2] = c;
	e[3] = d;
	eventhead = event;
}

uint32_t * GetUEvent()
{
	int event = eventtail;
	if( eventhead == event ) return 0;
	eventtail = ( event + 1 ) & (NUMUEVENTS-1);
	return &events[event*4];
}


int main()
{
	SystemInit();

	usb_setup();
	
	WS2812BDMAInit();

	while(1)
	{
		//printf( "%lu %lu %lu %08lx\n", rv003usb_internal_data.delta_se0_cyccount, rv003usb_internal_data.last_se0_cyccount, rv003usb_internal_data.se0_windup, RCC->CTLR );
		uint32_t * ue = GetUEvent();
		if( ue )
		{
			printf( "%lu %lx %lx %lx\n", ue[0], ue[1], ue[2], ue[3] );
		}
	}
}

void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	// Make sure we only deal with control messages.  Like get/set feature reports.
	if( endp )
	{
		usb_send_empty( sendtok );
	}
	else
	{
		int offset = (e->count)<<3;
		int remain = (int)e->max_len - (int)offset;
		if( remain <= 0 )
		{
			usb_send_empty( sendtok );
		}
		else
		{
			if( remain > 8 ) remain = 8;
			usb_send_data( scratch + offset, remain, 0, sendtok );
			// Don't advance, that will be done by ACK packets.
		}
	}
}

void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
	//LogUEvent( SysTick->CNT, current_endpoint, e->count, 0xaaaaaaaa );
	int offset = e->count<<3;
	int torx = e->max_len - offset;
	if( torx > len ) torx = len;
	if( torx > 0 )
	{
		memcpy( scratch + offset, data, torx );
		e->count++;
		if( ( e->count << 3 ) >= e->max_len )
		{
			WS2812BDMAStart( e->max_len/3 );
		}
	}
}

void usb_handle_hid_get_report_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
	if( reqLen > sizeof( scratch ) ) reqLen = sizeof( scratch );
	e->opaque = scratch;
	e->is_descriptor = 1;
	e->max_len = reqLen;
}

void usb_handle_hid_set_report_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
	if( reqLen > sizeof( scratch ) ) reqLen = sizeof( scratch );
	e->max_len = reqLen;
}


void usb_handle_other_control_message( struct usb_endpoint * e, struct usb_urb * s )
{
	LogUEvent( SysTick->CNT, s->wRequestTypeLSBRequestMSB, s->lValueLSBIndexMSB, s->wLength );
}


