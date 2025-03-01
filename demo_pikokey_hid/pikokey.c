#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"
#include "help_functions.h"
// ./minichlink -D to use nrst, also PD1 needs pullup 

int main()
{
	SystemInit();
	Delay_Ms(1); // Ensures USB re-enumeration after bootloader or reset; Spec demand >2.5µs ( TDDIS )
	systick_init();
	GPIO_Init_All();
    ButtonMatrix_Init(&btn_matrix); // Initialize button matrix state
    Debounce_Init(&debounce_info); // Initialize debounce info
	// printf( "Start\n");

	usb_setup();
	while(1)
	{
#if RV003USB_EVENT_DEBUGGING
		uint32_t * ue = GetUEvent();
		if( ue )
		{
			printf( "%lu %lx %lx %lx\n", ue[0], ue[1], ue[2], ue[3] );
		}
#endif
        uint32_t current_time = GetSystemTime();

        // Scan the button matrix with debouncing
        ButtonMatrix_Scan_Debounced(&btn_matrix, &debounce_info, current_time);
        Delay_Ms(10);

	}
}
int i=0;
uint8_t tsajoystick[8] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	if( endp == 1 )
	{
		i++;
		if(keypressed[0] == 1){
			tsajoystick[0] = keypressed[1];
			for(uint8_t k=0;k<6;k++){
				tsajoystick[2+k] = keypressed[2+k];
				keypressed[2+k]=0;
			}
			keypressed[1] = 0;
			keypressed[0] = 0;
		}
		usb_send_data( tsajoystick, 8, 0, sendtok );
		for(uint8_t k=0;k<8;k++){
			tsajoystick[k] = 0;
		}
		}
	else
	{
		usb_send_empty( sendtok );
	}
}


