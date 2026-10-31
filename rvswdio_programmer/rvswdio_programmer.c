#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

// For RVBB_REMAP
//        PC1+PC2 = SWIO (Plus 5.1k pull-up to PD2)
// Otherwise (Ideal for 8-pin SOIC, because PC4+PA1 are bound)
//        PC4+PA1 = SWIO (Plus 5.1k pull-up to PD2)
//
// By Default:
//        PD2 = Power control
//
// If programming V2xx, V3xx, X0xx, DEFAULT:
//        PC5 = SWCLK
//
// If you are using the MCO feature
//        PC4 = MCO (optional)
//
#define RVBB_REMAP                1 // To put SWD on PC1/PC2

#define PIN_SWCLK                 PC5 // Remappable.
#define PIN_TARGETPOWER           PD2 // Remappable
#define POWER_ON_POLARITY         1
#define POWER_OFF_POLARITY        0
#define PIN_TARGETPOWER_MODE      GPIO_CFGLR_OUT_10Mhz_PP

#define STATUS_LED                PC0

#ifdef STATUS_LED
#define STATUS_LED_OFF_POLARITY   0
#define STATUS_LED_ON_POLARITY    1
#endif

// Allow reading and writing to the scratchpad via HID control messages.
__attribute__ ((aligned(4))) uint8_t scratch[264];
__attribute__ ((aligned(4))) uint8_t retbuff[264];
volatile uint32_t scratch_run = 0;
volatile uint32_t scratch_return = 0;

#include "funprog_config.h"

#include "funprog_port.h"

#include "../ch32fun/minichlink/funprog/funprog_rvswdio.h"

#include "bitbang_rvswdio_ch32v003.h"

#if CH5xx_SUPPORT
#include "../ch32fun/minichlink/funprog/funprog_ch5xx.h"
#endif

#include "../ch32fun/minichlink/funprog/funprog_logic.h"

int main()
{
	SystemInit();
	Delay_Ms(1); // Ensures USB re-enumeration after bootloader or reset; Spec demand >2.5µs ( TDDIS )
	usb_setup();

	funGpioInitAll();

	ConfigureIOForRVSWIO( &state );
#ifdef STATUS_LED
	funPinMode( STATUS_LED, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP );
	funDigitalWrite( STATUS_LED, STATUS_LED_OFF_POLARITY );
#endif

	Delay_Ms(10);

#if 0
	// This tests the system to see if we can bring it up.
	funDigitalWrite( PD2, 1 );
	Delay_Ms(1);

	int k;
	for( k = 0; k < 10; k++ )
	{
		printf( "Sending\n" );
		MCFWriteReg32( &state, DMSHDWCFGR, 0x5aa50000 | (1<<10) ); // Shadow Config Reg
		MCFWriteReg32( &state, DMCFGR, 0x5aa50000 | (1<<10) );     // CFGR (1<<10 == Allow output from slave)
		MCFWriteReg32( &state, DMSHDWCFGR, 0x5aa50000 | (1<<10) ); // Shadow Config Reg
		MCFWriteReg32( &state, DMCFGR, 0x5aa50000 | (1<<10) );     // CFGR (1<<10 == Allow output from slave)
		MCFWriteReg32( &state, DMCONTROL, 0x80000001 );            // Make the debug module work properly.
		MCFWriteReg32( &state, DMCONTROL, 0x80000001 );            // Re-initiate a halt request.

		uint32_t reg;
		int ret = MCFReadReg32( &state, DMSTATUS, &reg );

		printf( "%08x %d\n", (int)reg, ret );
	}
#endif

	while(1)
	{
		//printf( "%lu %lu %lu %08lx\n", rv003usb_internal_data.delta_se0_cyccount, rv003usb_internal_data.last_se0_cyccount, rv003usb_internal_data.se0_windup, RCC->CTLR );
#if RV003USB_EVENT_DEBUGGING
		uint32_t * ue;
		while( ( ue = GetUEvent() ) )
		{
			printf( "%lu %lx %lx %lx\n", ue[0], ue[1], ue[2], ue[3] );
		}
#endif
		if( scratch_run )
		{
			scratch_return = 0;
			HandleCommandBuffer( scratch, (sizeof(scratch)-1), retbuff, (sizeof(retbuff)-1) );
			scratch_run = 0;
			scratch_return = 1;
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
	else
	{
		LogUEvent( 5, endp, sendtok, scratch_run );
		if( scratch_run ) 
		{
			// Send NACK (can't accept any more data right now)
			usb_send_data( 0, 0, 2, 0x5A );
		}
		else
		{
			if( !e->max_len )
			{
				usb_send_empty( sendtok );
			}
			else
			{
				int len = e->max_len;
				int slen = len;
				if( slen > 8 ) slen = 8;
				usb_send_data( (uint8_t*)e->opaque, slen, 0, sendtok );
				e->opaque += 8;
				len -= 8;
				if( len < 0 ) len = 0;
				e->max_len = len;
				LogUEvent( 6, e->max_len, slen, len );
			}
		}
	}
}


void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
	if( scratch_run ) 
	{
		// Send NACK (can't accept any more data right now)
		usb_send_data( 0, 0, 2, 0x5A );
		return;
	}

	usb_send_data( 0, 0, 2, 0xD2 ); // Send ACK
	int offset = e->count<<3;
	int torx = e->max_len - offset;
	if( torx > len ) torx = len;
	if( torx > 0 )
	{
		memcpy( scratch + offset, data, torx );
		e->count++;
		if( ( e->count << 3 ) >= e->max_len )
		{
			scratch_run = e->max_len;
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

	e->opaque = retbuff;
	e->max_len = reqLen;
	e->custom = 1;
	scratch_return = 0;
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

	e->opaque = scratch;
	e->custom = 0;
	if( scratch_run ) reqLen = 0;
	if( reqLen > sizeof( scratch ) ) reqLen = sizeof( scratch );
	e->max_len = reqLen;
	LogUEvent( 2, reqLen, lValueLSBIndexMSB, 0 );
}
