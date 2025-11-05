#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

int main()
{
	SystemInit();
	Delay_Ms(1); // Ensures USB re-enumeration after bootloader or reset; Spec demand >2.5µs ( TDDIS )
	usb_setup();
	while(1) {
#if RV003USB_EVENT_DEBUGGING
		uint32_t * ue = GetUEvent();
		if( ue )
		{
			printf( "%lu %lx %lx %lx\n", ue[0], ue[1], ue[2], ue[3] );
		}
#endif
	};
}

void usb_handle_other_control_message( struct usb_endpoint * e, struct usb_urb * s, struct rv003usb_internal * ist )
{
	LogUEvent( SysTick->CNT, s->wRequestTypeLSBRequestMSB, s->lValueLSBIndexMSB, s->wLength );
}


uint8_t off = 0;
static uint8_t tsajoystick[20] = { 0x00, 0x14, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	LogUEvent(endp, 0, 0, off);
	if(endp == 1)
	{

		if (!off) {
			tsajoystick[2]++;
			tsajoystick[4]++;
			tsajoystick[5]++;
			(*(int16_t*)(tsajoystick+6))+=100;
			(*(int16_t*)(tsajoystick+8))+=100;
			(*(int16_t*)(tsajoystick+10))-=100;
			(*(int16_t*)(tsajoystick+12))-=100;
		}

		uint32_t size = 8;
		if (off == 16) {
			size = 4;
		}

		usb_send_data(tsajoystick+off, size, 0, sendtok );

		off += 8;
		if (off > 16) {
			off = 0;
		}

	}
	else
	{
		// If it's a control transfer, nak it.
		usb_send_empty( sendtok );
	}
}