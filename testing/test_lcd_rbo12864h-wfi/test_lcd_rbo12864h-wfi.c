// For use with a ST7567 driver in an LCD.

#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

// Allow reading and writing to the scratchpad via HID control messages.
uint8_t scratch[1060];
int start_data;

#define LCDLED PC0
#define LCDVDD PC7
#define LCDA0  PC4
#define LCDRST PC3
#define LCDCSB PC2
#define LCDSDA PC6
#define LCDSCL PC5

void LCDCMD( int is_data, const uint8_t * data, int len )
{
	funDigitalWrite( LCDA0, is_data );
	funDigitalWrite( LCDCSB, 0 );
	int b;
	for( b = 0; b < len; b++ )
	{
		int i;
		uint8_t d = data[b];
		for( i = 0; i < 8; i++ )
		{
			funDigitalWrite( LCDSCL, 0 );
			funDigitalWrite( LCDSDA, !!((d<<i)&0x80) );
			funDigitalWrite( LCDSCL, 1 );
		}
	}
	funDigitalWrite( LCDSCL, 0 );
	funDigitalWrite( LCDCSB, 1 );
}

//------------------------------
// ST7567 Commands
#define ST7567_POWER_ON         0x2F // internal power supply on
#define ST7567_POWER_CTL        0x28 // internal power supply off
#define ST7567_CONTRAST         0x80 // 0x80 + (0..31)
#define ST7567_SEG_NORMAL       0xA0 // SEG remap normal
#define ST7567_SEG_REMAP        0xA1 // SEG remap reverse (flip horizontal)
#define ST7567_DISPLAY_NORMAL   0xA4 // display ram content
#define ST7567_DISPLAY_TEST     0xA5 // all pixels on
#define ST7567_INVERT_OFF       0xA6 // not inverted
#define ST7567_INVERT_ON        0xA7 // inverted
#define ST7567_DISPLAY_ON       0XAF // display on
#define ST7567_DISPLAY_OFF      0XAE // display off
#define ST7567_STATIC_OFF       0xAC
#define ST7567_STATIC_ON        0xAD
#define ST7567_SCAN_START_LINE  0x40 // scrolling 0x40 + (0..63)
#define ST7567_COM_NORMAL       0xC0 // COM remap normal
#define ST7567_COM_REMAP        0xC8 // COM remap reverse (flip vertical)
#define ST7567_SW_RESET         0xE2 // connect RST pin to GND and rely on software reset
#define ST7567_NOP              0xE3 // no operation
#define ST7567_TEST             0xF0

#define ST7567_COL_ADDR_H       0x10 // x pos (0..95) 4 MSB
#define ST7567_COL_ADDR_L       0x00 // x pos (0..95) 4 LSB
#define ST7567_PAGE_ADDR        0xB0 // y pos, 8.5 rows (0..8)
#define ST7567_RMW              0xE0
#define ST7567_RMW_CLEAR        0xEE

#define ST7567_BIAS_9           0xA2 
#define ST7567_BIAS_7           0xA3

#define ST7567_VOLUME_FIRST     0x81
#define ST7567_VOLUME_SECOND    0x00

#define ST7567_RESISTOR_RATIO   0x20
#define ST7567_STATIC_REG       0x0
#define ST7567_BOOSTER_FIRST    0xF8
#define ST7567_BOOSTER_234      0
#define ST7567_BOOSTER_5        1
#define ST7567_BOOSTER_6        3

const uint8_t init_data[] = {
	ST7567_BIAS_7,
	ST7567_SEG_NORMAL,
	ST7567_COM_REMAP,
	ST7567_POWER_CTL | 0x4,
	ST7567_POWER_CTL | 0x6, // turn on voltage regulator (VC=1, VR=1, VF=0)
	ST7567_POWER_CTL | 0x7, // turn on voltage follower (VC=1, VR=1, VF=1)
	ST7567_RESISTOR_RATIO | 0x3, // set lcd operating voltage (regulator resistor, ref voltage resistor)
	ST7567_SCAN_START_LINE+0,
	ST7567_DISPLAY_ON,
	ST7567_DISPLAY_NORMAL
};

int main()
{
	SystemInit();
	
	funGpioInitAll();
	funPinMode( LCDLED, GPIO_CFGLR_OUT_50Mhz_AF_PP );
	funPinMode( LCDVDD, GPIO_CFGLR_OUT_50Mhz_PP );
	funPinMode( LCDA0, GPIO_CFGLR_OUT_50Mhz_PP );
	funPinMode( LCDCSB, GPIO_CFGLR_OUT_50Mhz_PP );
	funPinMode( LCDSDA, GPIO_CFGLR_OUT_50Mhz_PP );
	funPinMode( LCDSCL, GPIO_CFGLR_OUT_50Mhz_PP );
	funPinMode( LCDRST, GPIO_CFGLR_OUT_50Mhz_PP );

	funDigitalWrite( LCDRST, 0 );
	funDigitalWrite( LCDVDD, 1 );
	funDigitalWrite( LCDA0, 0 );
	funDigitalWrite( LCDCSB, 1 );
	funDigitalWrite( LCDSDA, 1 );
	funDigitalWrite( LCDSCL, 1 );
	funDigitalWrite( LCDRST, 1 );

	// Enable Timer for PWM of LED pin.
	RCC->APB2PCENR |= RCC_APB2Periph_TIM1 | RCC_APB2Periph_AFIO;
	AFIO->PCFR1 |= AFIO_PCFR1_TIM1_REMAP_0;
	TIM1->PSC = 0x0001; // Set Prescalar
	TIM1->ATRLR = 255; // Set Period
	TIM1->CCER = TIM_CC3E;
	TIM1->CHCTLR2 = TIM_OC3M_2 | TIM_OC3M_1;
	TIM1->CH3CVR = 64; // Set to 25% brightness
	TIM1->BDTR = TIM_MOE;
	TIM1->CTLR1 = TIM_CEN;

	Delay_Ms(1); // Ensures USB re-enumeration after bootloader or reset; Spec demand >2.5µs ( TDDIS )
	usb_setup();

	LCDCMD( 0, init_data, sizeof(init_data) );

	while(1)
	{
		//printf( "%lu %lu %lu %08lx\n", rv003usb_internal_data.delta_se0_cyccount, rv003usb_internal_data.last_se0_cyccount, rv003usb_internal_data.se0_windup, RCC->CTLR );
#if RV003USB_EVENT_DEBUGGING
		uint32_t * ue = GetUEvent();
		if( ue )
		{
			printf( "%lu %lx %lx %lx\n", ue[0], ue[1], ue[2], ue[3] );
		}
#endif

#if 1
		if( start_data )
		{
			int group;
			uint8_t * sp = scratch + 1;
			TIM1->CH3CVR = *(sp++); // Set backlight
			LCDCMD( 0, sp, 4 );
			sp += 4;
			for( group = 0; group < 8; group++ )
			{
				LCDCMD( 0, sp, 4 );
				sp += 4;
				LCDCMD( 1, sp, 128 );
				sp += 128;
			}

		}
#endif
	}
}

void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	// Make sure we only deal with control messages.  Like get/set feature reports.
	if( endp )
	{
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
		memcpy( scratch + offset, data, torx );
		e->count++;
		if( ( e->count << 3 ) >= e->max_len )
		{
			start_data = e->max_len;
		}
	}
}

void usb_handle_hid_get_report_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
	if( reqLen > sizeof( scratch ) ) reqLen = sizeof( scratch );

	// You can check the lValueLSBIndexMSB word to decide what you want to do here
	// But, whatever you point this at will be returned back to the host PC where
	// it calls hid_get_feature_report. 
	//
	// Please note, that on some systems, for this to work, your return length must
	// match the length defined in HID_REPORT_COUNT, in your HID report, in usb_config.h

	e->opaque = scratch;
	e->max_len = reqLen;
}

void usb_handle_hid_set_report_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
	// Here is where you get an alert when the host PC calls hid_send_feature_report.
	//
	// You can handle the appropriate message here.  Please note that in this
	// example, the data is chunked into groups-of-8-bytes.
	//
	// Note that you may need to make this match HID_REPORT_COUNT, in your HID
	// report, in usb_config.h

	if( reqLen > sizeof( scratch ) ) reqLen = sizeof( scratch );
	e->max_len = reqLen;
}


void usb_handle_other_control_message( struct usb_endpoint * e, struct usb_urb * s, struct rv003usb_internal * ist )
{
	LogUEvent( SysTick->CNT, s->wRequestTypeLSBRequestMSB, s->lValueLSBIndexMSB, s->wLength );
}


