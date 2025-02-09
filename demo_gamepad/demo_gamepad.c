#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

int main()
{
	SystemInit();
	Delay_Ms(1); // Ensures USB re-enumeration after bootloader or reset; Spec demand >2.5µs ( TDDIS )
	usb_setup();
	while(1);
}

void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	if( endp )
	{
		static uint8_t tsajoystick[8] = { 0x00, 0x01, 0x10, 0x00 };
		tsajoystick[0]++;  // Go left->right fast
		tsajoystick[2] = tsajoystick[0] & 0x80; // Alter button 1.
		usb_send_data( tsajoystick, 3, 0, sendtok );
	}
	else
	{
		// If it's a control transfer, nak it.
		usb_send_empty( sendtok );
	}
}


