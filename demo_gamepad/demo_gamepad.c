// Could be defined here, or in the processor defines.
#define SYSTEM_CORE_CLOCK 48000000
#define SYSTICK_USE_HCLK

#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>

#define INSTANCE_DESCRIPTORS
#include "rv003usb.h"


int main()
{
	SystemInit();
	usb_setup();

	while(1)
	{
		printf( "%lu %lu %lu %08lx\n", rv003usb_internal_data.delta_se0_cyccount, rv003usb_internal_data.last_se0_cyccount, rv003usb_internal_data.se0_windup, RCC->CTLR );
	}
}

void usb_hande_interrupt_in( struct usb_endpoint * e, uint8_t * scratchpad, uint32_t sendtok )
{
	static uint8_t tsajoystick[8] = { 0x00, 0x01, 0x10, 0x00 };
	tsajoystick[0]++;
	tsajoystick[2]^=1; // Alter button 1.
	usb_send_data( tsajoystick, 3, 0, sendtok );
}

void usb_handle_control_in( struct usb_endpoint * e, uint8_t * scratchpad, uint32_t sendtok )
{
	// Not handling custom commands.
	usb_send_nak( sendtok );
}

void usb_handle_control_out( struct usb_endpoint * e, uint8_t * data, int len )
{
	// Not handling custom commands.
}

void usb_handle_control_out_start( struct usb_endpoint * e, int reqLen )
{
	// Not handling custom commands.
}

void usb_handle_control_in_start( struct usb_endpoint * e, int reqLen )
{
	// Not handling custom commands.
}

