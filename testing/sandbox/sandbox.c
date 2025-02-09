#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

int main()
{
	SystemInit();
	usb_setup();
	while(1);
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


