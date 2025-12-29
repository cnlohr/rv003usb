#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

#define WSRAW
#define WS2812B_ALLOW_INTERRUPT_NESTING
#define WS2812DMA_IMPLEMENTATION
#define DMALEDS 96 // Provide enough of a buffer things don't get messed up (actually uses 768 bytes)
#include "ws2812b_dma_spi_led_driver.h"

// Allow reading and writing to the scratchpad via HID control messages.
uint8_t scratch[255];
volatile uint8_t start_leds = 0;
uint8_t ledat;

static uint8_t frame;

volatile uint32_t another_interrupt_flag = 0;

uint32_t WS2812BLEDCallback( int wordno )
{
	return ((uint32_t*)scratch)[wordno+1];
}

void do_tricks() __attribute__((noinline));	// Makes it easier to find assembly of your code in lst
void do_tricks() {
	if (another_interrupt_flag != 0) {
		if (funDigitalRead(PC0)) {
			funDigitalWrite(PC0, 0);
		} else {
			funDigitalWrite(PC0, 1);
		}
		another_interrupt_flag = 0;
	}
}

int main()
{
	SystemInit();
	Delay_Ms(1); // Ensures USB re-enumeration after bootloader or reset; Spec demand >2.5µs ( TDDIS )
	usb_setup();

	GPIOD->CFGLR = ( ( GPIOD->CFGLR ) & (~( 0xf << (4*2) )) ) | 
		(GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*2);
	
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
	funPinMode( PC0, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP );
	funPinMode( PC7, GPIO_CFGLR_IN_PUPD );
	funDigitalWrite( PC7, 0 );	// Enable pull-down for external interrupt
	funDigitalWrite( PC0, 1 );	// Pin to toggle for demonstration

	AFIO->EXTICR |= AFIO_EXTICR1_EXTI7_PC;
	EXTI->INTENR |= EXTI_INTENR_MR7; // Enable EXT7
	EXTI->RTENR |= EXTI_RTENR_TR7;  // Rising edge trigger

	WS2812BDMAInit();// Use DMA and SPI to stream out WS2812B LED Data via the MOSI pin.
	Delay_Ms(500);

	while(1)
	{
		do_tricks();

#if RV003USB_EVENT_DEBUGGING
		uint32_t * ue = GetUEvent();
		if( ue )
		{
			printf( "%lu %lx %lx %lx\n", ue[0], ue[1], ue[2], ue[3] );
		}
#endif
		if( start_leds )
		{
			ledat = 3;
			if( scratch[1] == 0xa4 )
			{
				WS2812BDMAStart( 254/4 );
			}
			start_leds = 0;
			frame++;
		}
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
			start_leds = e->max_len;
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


