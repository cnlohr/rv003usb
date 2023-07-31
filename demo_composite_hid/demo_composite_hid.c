#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

int main()
{
	SystemInit();
	usb_setup();
	while(1);
}

void usb_handle_user_in( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	if( endp == 1 )
	{
		// Mouse (4 bytes)
		static int i;
		static uint8_t tsajoystick[4] = { 0x00, 0x00, 0x00, 0x00 };
		i++;
		int mode = i >> 5;

		// Move the mouse right, down, left and up in a square.
		switch( mode & 3 )
		{
		case 0: tsajoystick[1] =  1; tsajoystick[2] = 0; break;
		case 1: tsajoystick[1] =  0; tsajoystick[2] = 1; break;
		case 2: tsajoystick[1] = -1; tsajoystick[2] = 0; break;
		case 3: tsajoystick[1] =  0; tsajoystick[2] =-1; break;
		}
		usb_send_data( tsajoystick, 4, 0, sendtok );
	}
	else if( endp == 2 )
	{
		// Keyboard (8 bytes)b
		static int i;
		static uint8_t tsajoystick[8] = { 0x00 };
		usb_send_data( tsajoystick, 8, 0, sendtok );

		i++;

		// Press the 'b' button every second or so.
		if( (i & 0x7f) == 0 )
		{
			tsajoystick[4] = 5;
		}
		else
		{
			tsajoystick[4] = 0;
		}
	}
	else
	{
		// If it's a control transfer, empty it.
		usb_send_empty( sendtok );
	}
}


