#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"
#include "ch32v003_touch.h"

#define BTN_LEFT   1
#define BTN_RIGHT  6
#define BTN_UP     0
#define BTN_DOWN   5
#define BTN_SELECT 7
#define BTN_START  4
#define BTN_A      2
#define BTN_B      3


uint16_t ttval[8];
uint16_t base[8];

struct HostData
{
	uint8_t  report;
	uint8_t  extra;
	int16_t  caled[8];
} host;
uint32_t frame;

uint8_t tsajoystick[3];

volatile void ReadVals() __attribute__ ((noinline)) __attribute__((noinline, section(".srodata")));;
void ReadVals()
{
	#define iterations 5
	asm volatile( ".balign 16\n" );

	// Then do this any time you want to read some touches.
	ttval[0] = ReadTouchPin( GPIOA, 2, 0, iterations );
	ttval[1] = ReadTouchPin( GPIOA, 1, 1, iterations );
	ttval[2] = ReadTouchPinSafe( GPIOC, 4, 2, iterations );
	ttval[3] = ReadTouchPin( GPIOD, 2, 3, iterations );
	ttval[4] = ReadTouchPin( GPIOD, 3, 4, iterations );
	ttval[5] = ReadTouchPin( GPIOD, 5, 5, iterations );
	ttval[6] = ReadTouchPin( GPIOD, 6, 6, iterations );
	ttval[7] = ReadTouchPin( GPIOD, 4, 7, iterations );
}

int main()
{
	SystemInit();

	// Enable GPIOD, C and ADC
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC | RCC_APB2Periph_ADC1;

	InitTouchADC();

	ReadVals();
	ReadVals();
	memcpy( base, ttval, sizeof( ttval ) );

	usb_setup();

	while(1)
	{
		ReadVals();

		int i;

		uint32_t btns = 0;
		for( i = 0; i < 8; i++ )
		{
			uint32_t val = ttval[i];
			uint32_t b = base[i];
			base[i] = b;
			int c = val - b;
			host.caled[i] = c;
			if( c > 0x800 )
				btns |= 1<<i;
		}


		{
			int l = host.caled[BTN_LEFT];
			int r = host.caled[BTN_RIGHT];
			int u = host.caled[BTN_UP];
			int d = host.caled[BTN_DOWN];
			if( l > r )
				tsajoystick[0] = (-l)>>5;
			else
				tsajoystick[0] = (r)>>5;

			if( u > d )
				tsajoystick[1] = (-u)>>5;
			else
				tsajoystick[1] = (d)>>5;
			tsajoystick[2] = btns;
		}

		frame++;

	}

}

void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	if( endp )
	{
		usb_send_data( tsajoystick, 3, 0, sendtok );
	}
	else
	{
		// If it's a control transfer, nak it.
		usb_send_empty( sendtok );
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
		// for getting data host->us.

		//memcpy( caled + offset, data, torx );
		e->count++;
		//if( ( e->count << 3 ) >= e->max_len )
		//{
		//	start_leds = e->max_len;
		//}
	}
}

void usb_handle_hid_get_report_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
	if( reqLen > sizeof( host ) ) reqLen = sizeof( host );
	e->opaque = ((uint8_t*)&host);
	e->max_len = reqLen;
}

void usb_handle_hid_set_report_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
	if( reqLen > sizeof( host ) ) reqLen = sizeof( host );
	e->max_len = reqLen;
}


