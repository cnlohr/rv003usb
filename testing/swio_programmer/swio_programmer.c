#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

// Allow reading and writing to the scratchpad via HID control messages.
uint8_t scratch[255];

static uint8_t frame;

uint32_t command_buffer[128];
uint8_t reply_buffer[128];

int main()
{
	int i;

	SystemInit();
	Delay_Ms(1); // Ensures USB re-enumeration after bootloader or reset; Spec demand >2.5µs ( TDDIS )
	usb_setup();

	
	funGpioInitAll();

	// Assume PD0 = SWIO
	//        PD2 = Power control

	RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;
	RCC->APB2PCENR |= RCC_APB2Periph_TIM1;


	funPinMode( PD0, GPIO_CFGLR_OUT_10Mhz_PP );
	funPinMode( PD2, GPIO_CFGLR_OUT_10Mhz_PP );


	// Fill in the plan of what we will be sending out.
	for( i = 0; i < sizeof(command_buffer) / sizeof(command_buffer[0]); i++ )
	{
		command_buffer[i] = 1 << ( (i & 1) ? 16 : 0 );
	}


	// Timer 1 setup.
	// Timer 1 is what will trigger the DMA, Channel 2 engine.
	TIM1->PSC = 0x3110;                      // Prescaler 
	TIM1->ATRLR = 48;                        // Auto Reload - sets period (48MHz / (11+1) = 4MHz)
	TIM1->SWEVGR = TIM_UG | TIM_TG;          // Reload immediately + Trigger DMA
	TIM1->CCER = TIM_CC1E | TIM_CC1P | TIM_CC2E | TIM_CC2P;        // Enable CH1 output, positive pol
	TIM1->CHCTLR1 = TIM_OC1M_2 | TIM_OC1M_1 | TIM_OC2M_2 | TIM_OC2M_1; // CH1 Mode is output, PWM1 (CC1S = 00, OC1M = 110)
	TIM1->CH1CVR = 6;                        // Time to send update
	TIM1->CH2CVR = 24;                       // Time point to read bit back
	TIM1->BDTR = TIM_MOE;                    // Enable TIM1 outputs
	TIM1->CTLR1 = TIM_CEN;                   // Enable TIM1
	TIM1->DMAINTENR = TIM_UDE | TIM_CC1DE | TIM_CC2DE;   // Trigger DMA on TC match 1 (DMA Ch2) and TC match 2 (DMA Ch3)


	DMA1_Channel2->MADDR = (uint32_t)command_buffer;
	DMA1_Channel2->PADDR = (uint32_t)&GPIOD->BSHR; // This is the output register for out buffer.
	DMA1_Channel2->CFGR = 
		DMA_CFGR1_DIR |                      // MEM2PERIPHERAL
		DMA_CFGR1_PL |                       // High priority.
		DMA_CFGR1_PSIZE_1 |                  // 32-bit memory
		DMA_CFGR1_MSIZE_1 |                  // 32-bit peripheral
		DMA_CFGR1_MINC |                     // Increase memory.
		0              |                     // NOT Circular mode.
		0              |                     // NO Half-trigger
		0              |                     // NO Whole-trigger
		DMA_CFGR1_EN;                        // Enable

	DMA1_Channel3->MADDR = (uint32_t)reply_buffer;
	DMA1_Channel3->PADDR = (uint32_t)&GPIOD->INDR; // This is the output register for out buffer.
	DMA1_Channel3->CFGR = 
		0 |                                  // PERIPHERAL to MEMORY
		DMA_CFGR1_PL |                       // High priority.
		0 |                                  // 8-bit memory
		0 |                                  // 8-bit peripheral
		DMA_CFGR1_MINC |                     // Increase memory.
		0              |                     // NOT Circular mode.
		0              |                     // NO Half-trigger
		0              |                     // NO Whole-trigger
		DMA_CFGR1_EN;                        // Enable

	DMA1_Channel2->CNTR = sizeof(command_buffer) / sizeof(command_buffer[0]);
	DMA1_Channel3->CNTR = sizeof(reply_buffer) / sizeof(reply_buffer[0]);

	while( DMA1_Channel2->CNTR );
	while( DMA1_Channel3->CNTR );

	for( i = 0; i < sizeof(reply_buffer) / sizeof(reply_buffer[0]); i++ )
	{
		printf( "%3d: %d\n", i, 1&(int)reply_buffer[i] );
	}

	GPIOD->BSHR = 1<<16;


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


