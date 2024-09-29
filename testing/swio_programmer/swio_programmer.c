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

uint8_t command_buffer[64];  // to timer2
uint8_t reply_buffer[64];    // from USART RX
static uint32_t buffptr = 0;

static void FinishBuffer();
static void Send1Bit();
static void Send0Bit();
static void IssueBuffer();

static void FinishBuffer()
{
	while(1)
	{
		printf( "%d %d: %02x %02x %02x %02x %02x %02x %02x %02x\n", DMA1_Channel2->CNTR, DMA1_Channel5->CNTR, reply_buffer[0], reply_buffer[1], reply_buffer[2], reply_buffer[3], reply_buffer[4], reply_buffer[5], reply_buffer[6], reply_buffer[7] );
	}
	while( DMA1_Channel2->CNTR ); // To Timer 2
	while( DMA1_Channel5->CNTR ); // From USART RX
	buffptr = 0;
}

#define TPERIOD 51

static void Send1Bit()
{
	// 224-236ns low pulse.
	command_buffer[buffptr++] = 3;
}

static void Send0Bit()
{
	// 862-876ns
	command_buffer[buffptr++] = 30;
}

static void IssueBuffer()
{
	command_buffer[buffptr] = 0; // Turn PWM back to high.
	command_buffer[buffptr+1] = 0; // Make sure the USART RX is read to read.
	DMA1_Channel2->CNTR = buffptr+2;
	DMA1_Channel5->CNTR = buffptr;
}


static void MCFWriteReg32( struct SWIOState * state, uint8_t command, uint32_t value )
{
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
	FinishBuffer();
}

// returns 0 if no error, otherwise error.
static int MCFReadReg32( struct SWIOState * state, uint8_t command, uint32_t * value )
{
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
	RCC->APB2PCENR |= RCC_APB2Periph_USART1;
	RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;
	RCC->APB1PCENR |= RCC_APB1Periph_TIM2;
	RCC->APB2PCENR |= RCC_APB2Periph_AFIO;

	// Reset TIM2 to init all regs
	RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
	RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;
	
	// SMCFGR: default clk input is CK_INT
	// set TIM2 clock prescaler divider 
	TIM2->PSC = 0x0000;
	// set PWM total cycle width
	TIM2->ATRLR = TPERIOD;

	TIM2->CH3CVR = 0;
	
	// for channel 1 and 2, let CCxS stay 00 (output), set OCxM to 110 (PWM I)
	// enabling preload causes the new pulse width in compare capture register only to come into effect when UG bit in SWEVGR is set (= initiate update) (auto-clears)
	TIM2->CHCTLR2 |= TIM_OC1M_2 | TIM_OC1M_1 | TIM_OC1PE; // TIM2 CH3

	TIM2->CTLR1 |= TIM_ARPE;

	// Enable CH1 output, positive pol
	TIM2->CCER |= TIM_CC3E | TIM_CC3P;

	// initialize counter
	TIM2->SWEVGR |= TIM_UG;

	// Enable TIM2
	TIM2->CTLR1 |= TIM_CEN;

	TIM2->DMAINTENR = TIM_UDE;

	// Enable tim2 dma
	TIM2->CTLR1 |= TIM_URS;   // Trigger DMA on overflow ONLY.

//R16_TIM2_DMACFGR
//R16_TIM2_DMAADR


TIM2->CH3CVR = 0;

	// Remap T2C3 to PD6
	AFIO->PCFR1 |= AFIO_PCFR1_TIM2_REMAP_FULLREMAP;


	// USART
	// Enable RX, 
	USART1->CTLR1 = USART_WordLength_8b | USART_Parity_No | USART_Mode_Rx | USART_Mode_Tx;
	USART1->CTLR2 = USART_StopBits_1;
	USART1->CTLR3 = USART_HardwareFlowControl_None | USART_CTLR3_DMAR;

	USART1->BRR = 1;
	USART1->CTLR1 |= CTLR1_UE_Set;


	USART1->DATAR = 0xaa;


#if 1

	DMA1_Channel2->MADDR = (uint32_t)command_buffer;
	DMA1_Channel2->PADDR = (uint32_t)&TIM2->CH3CVR; // This is the output register for out buffer.
	DMA1_Channel2->CFGR = 
		DMA_CFGR1_DIR     |                  // MEM2PERIPHERAL
		0                 |                  // Low priority.
		0                 |                  // 8-bit memory
		DMA_CFGR1_PSIZE_0                 |                  // 32-bit peripheral
		DMA_CFGR1_MINC    |                  // Increase memory.
		0                 |                  // NOT Circular mode.
		0                 |                  // NO Half-trigger
		0                 |                  // NO Whole-trigger
		DMA_CFGR1_EN;                        // Enable

#endif

#if 1
	DMA1_Channel5->MADDR = (uint32_t)reply_buffer;
	DMA1_Channel5->PADDR = (uint32_t)&USART1->DATAR; // This is the output register for out buffer.
	DMA1_Channel5->CFGR = 
		0                 |                  // PERIPHERAL to MEMORY
		0                 |                  // Low priority.
		0                 |                  // 8-bit memory
		0                 |                  // 8-bit peripheral
		DMA_CFGR1_MINC    |                  // Increase memory.
		0                 |                  // NOT Circular mode.
		0                 |                  // NO Half-trigger
		0                 |                  // NO Whole-trigger
		DMA_CFGR1_EN;                        // Enable
#endif


}



int main()
{
	int i;

	SystemInit();
	Delay_Ms(1); // Ensures USB re-enumeration after bootloader or reset; Spec demand >2.5µs ( TDDIS )
	usb_setup();

	
	funGpioInitAll();

	// Assume PD6+PA1 = SWIO
	//        PD2 = Power control

	// Fill in the plan of what we will be sending out.
	for( i = 0; i < sizeof(command_buffer) / sizeof(command_buffer[0]); i++ )
	{
		command_buffer[i] = i;
	}


	funDigitalWrite( PD2, 1 );
	funDigitalWrite( PD6, 1 );

	printf( "Starting\n" );

	funPinMode( PD6, GPIO_CFGLR_OUT_10Mhz_AF_OD );
	funPinMode( PD5, GPIO_CFGLR_OUT_10Mhz_AF_PP );
	funPinMode( PC4, GPIO_CFGLR_IN_FLOAT );
	funPinMode( PC4, GPIO_CFGLR );


	SetupTimer1AndDMA();


	Delay_Ms(30);

	printf( "Sending\n" );
	MCFWriteReg32( &sws, DMSHDWCFGR, 0x5aa50000 | (1<<10) ); // Shadow Config Reg
	MCFWriteReg32( &sws, DMCFGR, 0x5aa50000 | (1<<10) );     // CFGR (1<<10 == Allow output from slave)
	MCFWriteReg32( &sws, DMCONTROL, 0x80000001 );            // Make the debug module work properly.
	MCFWriteReg32( &sws, DMCONTROL, 0x80000003 );            // Reboot.
	MCFWriteReg32( &sws, DMCONTROL, 0x80000001 );            // Re-initiate a halt request.

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


