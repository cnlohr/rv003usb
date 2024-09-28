#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"


#define IRAM_ATTR
#include "ch32v003_swio.h"

struct SWIOState sws;

// Allow reading and writing to the scratchpad via HID control messages.
uint8_t scratch[255];

static uint8_t frame;

uint32_t command_buffer[328];
uint8_t reply_buffer[328];
static uint32_t buffptr = 0;


#define HIGH_RELEASE  (1 << ( (0 & 1) ? 16 : 0 ))
#define LOW_DRIVE     (1 << ( (1 & 1) ? 16 : 0 ))

static void FinishBuffer();
static void Send1Bit();
static void Send0Bit();
static void IssueBuffer();

static void FinishBuffer()
{
	while( DMA1_Channel2->CNTR );
	while( DMA1_Channel3->CNTR );
	buffptr = 0;
}

static void Send1Bit()
{
	// 224-236ns low pulse.
	command_buffer[buffptr++] = LOW_DRIVE;
	command_buffer[buffptr++] = HIGH_RELEASE;
	command_buffer[buffptr++] = HIGH_RELEASE;
	command_buffer[buffptr++] = HIGH_RELEASE;
	command_buffer[buffptr++] = HIGH_RELEASE;
	command_buffer[buffptr++] = HIGH_RELEASE;
}

static void Send0Bit()
{
	// 862-876ns
	command_buffer[buffptr++] = LOW_DRIVE;
	command_buffer[buffptr++] = LOW_DRIVE;
	command_buffer[buffptr++] = LOW_DRIVE;
	command_buffer[buffptr++] = LOW_DRIVE;
	command_buffer[buffptr++] = HIGH_RELEASE;
	command_buffer[buffptr++] = HIGH_RELEASE;
}

static void IssueBuffer()
{

	DMA1_Channel2->CNTR = buffptr;
	DMA1_Channel3->CNTR = buffptr;
}


static void MCFWriteReg32( struct SWIOState * state, uint8_t command, uint32_t value )
{
	FinishBuffer();

	Send1Bit();
	uint32_t mask;
	for( mask = 1<<6; mask; mask >>= 1 )
	{
		if( command & mask )
			Send1Bit();
		else
			Send0Bit();
	}
	Send1Bit( );
	for( mask = 1<<31; mask; mask >>= 1 )
	{
		if( value & mask )
			Send1Bit();
		else
			Send0Bit();
	}

	IssueBuffer();
}

// returns 0 if no error, otherwise error.
static int MCFReadReg32( struct SWIOState * state, uint8_t command, uint32_t * value )
{
	FinishBuffer();
	Send1Bit();
	uint32_t mask;
	for( mask = 1<<6; mask; mask >>= 1 )
	{
		if( command & mask )
			Send1Bit();
		else
			Send0Bit();
	}
	Send0Bit( );
	for( mask = 1<<31; mask; mask >>= 1 )
	{
		Send1Bit();
	}

	IssueBuffer();
	FinishBuffer();


	int i;
	for( i = 0; i < sizeof(reply_buffer) / sizeof(reply_buffer[0]); i++ )
	{
		printf( "%d", 1&(int)reply_buffer[i] );
	}
	printf( "\n" );

	return 0;
}


void SetupTimer1AndDMA()
{

	RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;
	RCC->APB2PCENR |= RCC_APB2Periph_TIM1;


	// Timer 1 setup.
	// Timer 1 is what will trigger the DMA, Channel 2 engine.
	TIM1->PSC = 0x0000;                      // Prescaler 
	TIM1->ATRLR = 4;                        // Auto Reload - sets period (48MHz / (11+1) = 4MHz)
	TIM1->SWEVGR = TIM_UG | TIM_TG;          // Reload immediately + Trigger DMA
	TIM1->CCER = TIM_CC1E | TIM_CC1P | TIM_CC2E | TIM_CC2P;        // Enable CH1 output, positive pol
	TIM1->CHCTLR1 = TIM_OC1M_2 | TIM_OC1M_1 | TIM_OC2M_2 | TIM_OC2M_1; // CH1 Mode is output, PWM1 (CC1S = 00, OC1M = 110)
	TIM1->CH1CVR = 1;                        // Time to send update
	TIM1->CH2CVR = 4;                       // Time point to read bit back
	TIM1->BDTR = TIM_MOE;                    // Enable TIM1 outputs
	TIM1->CTLR1 = TIM_CEN;                   // Enable TIM1
	TIM1->DMAINTENR = TIM_UDE | TIM_CC1DE | TIM_CC2DE;   // Trigger DMA on TC match 1 (DMA Ch2) and TC match 2 (DMA Ch3)

	DMA1_Channel2->MADDR = (uint32_t)command_buffer;
	DMA1_Channel2->PADDR = (uint32_t)&GPIOD->BSHR; // This is the output register for out buffer.
	DMA1_Channel2->CFGR = 
		DMA_CFGR1_DIR     |                  // MEM2PERIPHERAL
		DMA_CFGR1_PL      |                  // High priority.
		DMA_CFGR1_PSIZE_1 |                  // 32-bit memory
		DMA_CFGR1_MSIZE_1 |                  // 32-bit peripheral
		DMA_CFGR1_MINC    |                  // Increase memory.
		0                 |                  // NOT Circular mode.
		0                 |                  // NO Half-trigger
		0                 |                  // NO Whole-trigger
		DMA_CFGR1_EN;                        // Enable

	DMA1_Channel3->MADDR = (uint32_t)reply_buffer;
	DMA1_Channel3->PADDR = (uint32_t)&GPIOD->INDR; // This is the output register for out buffer.
	DMA1_Channel3->CFGR = 
		0                 |                  // PERIPHERAL to MEMORY
		0                 |                  // Low priority.
		0                 |                  // 8-bit memory
		0                 |                  // 8-bit peripheral
		DMA_CFGR1_MINC    |                  // Increase memory.
		0                 |                  // NOT Circular mode.
		0                 |                  // NO Half-trigger
		0                 |                  // NO Whole-trigger
		DMA_CFGR1_EN;                        // Enable

//	DMA1_Channel2->CNTR = sizeof(command_buffer) / sizeof(command_buffer[0]);
//	DMA1_Channel3->CNTR = sizeof(reply_buffer) / sizeof(reply_buffer[0]);
}



int main()
{
	int i;

	SystemInit();
	Delay_Ms(1); // Ensures USB re-enumeration after bootloader or reset; Spec demand >2.5µs ( TDDIS )
	usb_setup();

	
	funGpioInitAll();

	// Assume PD0 = SWIO
	//        PD2 = Power control



	// Fill in the plan of what we will be sending out.
	for( i = 0; i < sizeof(command_buffer) / sizeof(command_buffer[0]); i++ )
	{
		command_buffer[i] = 1 << ( (i & 1) ? 16 : 0 );
	}


	SetupTimer1AndDMA();

	funDigitalWrite( PD2, 1 );
	funDigitalWrite( PD0, 1 );

	funPinMode( PD0, GPIO_CFGLR_OUT_10Mhz_OD );
	funPinMode( PD2, GPIO_CFGLR_OUT_10Mhz_PP );
	Delay_Ms(30);

	MCFWriteReg32( &sws, DMSHDWCFGR, 0x5aa50000 | (1<<10) ); // Shadow Config Reg
	MCFWriteReg32( &sws, DMCFGR, 0x5aa50000 | (1<<10) ); // CFGR (1<<10 == Allow output from slave)
	MCFWriteReg32( &sws, DMCONTROL, 0x80000001 ); // Make the debug module work properly.
	MCFWriteReg32( &sws, DMCONTROL, 0x80000003 ); // Reboot.
	MCFWriteReg32( &sws, DMCONTROL, 0x80000001 ); // Re-initiate a halt request.

	uint32_t reg;
	int ret = MCFReadReg32( &sws, DMSTATUS, &reg );

	printf( "%08x %d\n", (int)reg, ret );

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
			//start_leds = e->max_len;
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


