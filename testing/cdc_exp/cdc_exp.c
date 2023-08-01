#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

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
	while(1)
	{
		uint32_t * ue = GetUEvent();
		if( ue )
		{
			printf( "%lu %lx %lx %lx\n", ue[0], ue[1], ue[2], ue[3] );
		}
	}
}

void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	if( endp == 3 )
	{
		usb_send_data( (uint8_t*)"Hello!~~", 8, 0, sendtok );
	}
	else if( endp == 1 )
	{
		usb_send_empty( sendtok );
	}
	else
	{
		// If it's a control transfer, nak it.
		usb_send_empty( sendtok );
	}
}


void usb_handle_other_control_message( struct usb_endpoint * e, struct usb_urb * s )
{
	LogUEvent( SysTick->CNT, s->wRequestTypeLSBRequestMSB, s->lValueLSBIndexMSB, s->wLength );
	e->opaque = 1;
}

void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
	LogUEvent( SysTick->CNT, 0xffffffff, current_endpoint, len );
}


