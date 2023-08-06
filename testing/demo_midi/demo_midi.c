#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

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
	LogUEvent( SysTick->CNT, endp, 0, -1 );
/*
	if( endp )
	{
		static uint8_t tsajoystick[8] = { 0x00, 0x01, 0x10, 0x00 };
		tsajoystick[0]++;  // Go left->right fast
		tsajoystick[2]^=1; // Alter button 1.
		usb_send_data( tsajoystick, 3, 0, sendtok );
	}
	else
	{
		// If it's a control transfer, nak it.
		usb_send_empty( sendtok );
	}
*/
}

void usb_handle_other_control_message( struct usb_endpoint * e, struct usb_urb * s, struct rv003usb_internal * ist )
{
	LogUEvent( SysTick->CNT, s->wRequestTypeLSBRequestMSB, s->lValueLSBIndexMSB, s->wLength );
}

void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
	LogUEvent( SysTick->CNT, current_endpoint, len, 0xaaaaaaaa );
}




