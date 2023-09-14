#include "ch32v003fun.h"
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


uint32_t ttval[8];
uint32_t base[8];
int16_t  caled[8];
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
		uint8_t btns = 0;
		for( i = 0; i < 8; i++ )
		{
			uint32_t val = ttval[i];
			uint32_t b = base[i];
			base[i] = b;
			int c = val - b;
			caled[i] = c;
			if( c > 0x800 )
				btns |= 1<<i;
		}


		{
			int l = caled[BTN_LEFT];
			int r = caled[BTN_RIGHT];
			int u = caled[BTN_UP];
			int d = caled[BTN_DOWN];
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

/*
		if( ( frame & 0xff ) == 0 )
		{
			printf( "%04x %04x %04x %04x %04x %04x %04x %04x\n",
				ttval[0], ttval[1], ttval[2], ttval[3],
				ttval[4], ttval[5], ttval[6], ttval[7] );
		}

*/
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


