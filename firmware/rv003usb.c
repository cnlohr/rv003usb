// Could be defined here, or in the processor defines.
#define SYSTEM_CORE_CLOCK 48000000
#define SYSTICK_USE_HCLK

#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>

#define INSTANCE_DESCRIPTORS
#include "rv003usb.h"

uint8_t scratchpad[128];

struct rv003usb_internal rv003usb_internal_data;

// This is the data actually required for USB.
uint8_t data_receptive;
/*
void usb_handle_custom_control( uint8_t bmRequestType, uint8_t bRequest, uint16_t wLength,  struct rv003usb_internal * ist )
{
	// Do something.
}*/

int main()
{
	SystemInit48HSI();
//	SetupDebugPrintf();
	SETUP_SYSTICK_HCLK

	// Enable GPIOs, TIMERs
	RCC->APB2PCENR = RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM1 | RCC_APB2Periph_GPIOA  | RCC_APB2Periph_AFIO | RCC_APB2Periph_TIM1;

// To use time debugging, enable thsi here, and DEBUG_TIMING in the .S
// You must update in tandem
#define DEBUG_TIMING

	// GPIO C0 Push-Pull
	GPIOC->CFGLR = (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*0) |
	               (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF)<<(4*3) | // PC3 = T1C3
#ifdef DEBUG_TIMING
	               (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF)<<(4*4) | // PC4 = T1C4
#endif
	               (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*2);

#ifdef DEBUG_TIMING
	{
		// PC4 is MCO (for watching timing)
		GPIOC->CFGLR &= ~(GPIO_CFGLR_MODE4 | GPIO_CFGLR_CNF4);
		GPIOC->CFGLR |= GPIO_CFGLR_CNF4_1 | GPIO_CFGLR_MODE4_0 | GPIO_CFGLR_MODE4_1;
		RCC->CFGR0 = (RCC->CFGR0 & ~RCC_CFGR0_MCO) | RCC_CFGR0_MCO_SYSCLK;

		// PWM is used for debug timing. 
		TIM1->PSC = 0x0000;

		// Auto Reload - sets period
		TIM1->ATRLR = 0xffff;

		// Reload immediately
		TIM1->SWEVGR |= TIM_UG;

		// Enable CH4 output, positive pol
		TIM1->CCER |= TIM_CC3E | TIM_CC3NP;

		// CH2 Mode is output, PWM1 (CC1S = 00, OC1M = 110)
		TIM1->CHCTLR2 |= TIM_OC3M_2 | TIM_OC3M_1;

		// Set the Capture Compare Register value to 50% initially
		TIM1->CH3CVR = 2;
		
		// Enable TIM1 outputs
		TIM1->BDTR |= TIM_MOE;
		
		// Enable TIM1
		TIM1->CTLR1 |= TIM_CEN;
	}
#endif

	const uint32_t trim = 15;
	uint32_t regtemp;
	regtemp = RCC->CTLR & ~RCC_HSITRIM;
	regtemp |= (trim<<3);
	RCC->CTLR = regtemp;

	// GPIO D3 for input pin change.
	GPIOD->CFGLR =
		(GPIO_CNF_IN_PUPD)<<(4*1) |  // Keep SWIO enabled.
		(GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*DEBUG_PIN) |
		(GPIO_SPEED_IN | GPIO_CNF_IN_PUPD)<<(4*USB_DM) |  //PD3 = GPIOD IN
		(GPIO_SPEED_IN | GPIO_CNF_IN_PUPD)<<(4*USB_DP) |  //PD4 = GPIOD IN
		(GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*USB_DPU);

	// Configure the IO as an interrupt.
	AFIO->EXTICR = 3<<(USB_DP*2); //PORTD.3 (3 out front says PORTD, 3 in back says 3)
	EXTI->INTENR = 1<<USB_DP; // Enable EXT3
	EXTI->FTENR = 1<<USB_DP;  // Rising edge trigger

	// This drive GPIO5 high, which will tell the host that we are going on-bus.
	GPIOD->BSHR = 1<<USB_DPU;

	// enable interrupt
	NVIC_EnableIRQ( EXTI7_0_IRQn );

	while(1);
}


#define ENDPOINT0_SIZE 8 //Fixed for USB 1.1, Low Speed.


//Received a setup for a specific endpoint.
void usb_pid_handle_setup( uint32_t addr, uint8_t * data, uint32_t endp, struct rv003usb_internal * ist )
{
	ist->current_endpoint = endp;
	struct usb_endpoint * e = &ist->eps[endp];

	e->toggle_out = 0;
	e->toggle_in = 1;
	e->count_in = 0;
	ist->setup_request = 1;

	TIM1->CNT = 0;
	TIM1->CNT = 0;
	TIM1->CNT = 0;
	TIM1->CNT = 0;
	TIM1->CNT = 0;
	TIM1->CNT = 0;
	TIM1->CNT = 0;
}

void usb_pid_handle_in( uint32_t addr, uint8_t * data, uint32_t endp, struct rv003usb_internal * ist )
{
	ist->current_endpoint = endp;
	struct usb_endpoint * e = &ist->eps[endp];

	int tosend = 0;
	uint8_t * sendnow = data-1;
	uint8_t * sendnowo = data-1;
	uint8_t sendtok = e->toggle_in?0b01001011:0b11000011;

		TIM1->CNT = 0;
		TIM1->CNT = 0;
		TIM1->CNT = 0;
		TIM1->CNT = 0;
	
	// Handle IN (sending data back to PC)
	// Do this down here.
	// We do this because we are required to have an in-endpoint.  We don't
	// have to do anything with it, though.
	if( endp )
	{
		TIM1->CNT = 0;
		goto send_nada;
	}

	uint8_t * tsend = 0;

	if( e->is_descriptor )
	{
		const struct descriptor_list_struct * dl = &descriptor_list[e->opaque];
		tsend = ((uint8_t*)dl->addr);
	}
	else if( e->opaque == 1 )
	{
		// Yes, it's a 0xAA
		tsend = scratchpad;
	}

	int offset = (e->count_in)<<3;
	tosend = ist->control_max_len - offset;
	if( tosend > ENDPOINT0_SIZE ) tosend = ENDPOINT0_SIZE;
	if( tosend < 0 ) tosend = 0;
	sendnow = tsend + offset;

	if( !tosend || !tsend )
	{
		goto send_nada;
	}
	else
	{
		usb_send_data( sendnow, tosend, 0, sendtok );
	}
	return;
send_nada:
	sendnowo[0] = 0;
	sendnowo[1] = 0; //CRC = 0
	usb_send_data( sendnowo, 2, 2, sendtok );  //DATA = 0, 0 CRC.
}

void usb_pid_handle_out( uint32_t addr, uint8_t * data, uint32_t endp, struct rv003usb_internal * ist )
{
	ist->current_endpoint = endp;


		TIM1->CNT = 0;
		TIM1->CNT = 0;
	asm volatile( "nop; nop; nop; nop;" );
		TIM1->CNT = 0;
		TIM1->CNT = 0;
	
	//We need to handle this here because we could have an interrupt in the middle of a control or big transfer.
	//This will correctly swap back the endpoint.
}

void usb_pid_handle_data( uint32_t this_token, uint8_t * data, uint32_t which_data, int32_t crc_ok, uint32_t length )
{
	TIM1->CNT = 0;
	TIM1->CNT = 0;
	TIM1->CNT = 0;
	//Received data from host.
	struct rv003usb_internal * ist = &rv003usb_internal_data;
	int cep = ist->current_endpoint;
	struct usb_endpoint * e = &ist->eps[cep];

	// Alrady received this packet.
	if( e->toggle_out != which_data )
	{
		TIM1->CNT = 0;
		TIM1->CNT = 0;
		TIM1->CNT = 0;
		TIM1->CNT = 0;
		TIM1->CNT = 0;
		if( cep == 0 )
		{
			TIM1->CNT = 0;
			TIM1->CNT = 0;
		}	
		goto just_ack;
	}

	e->toggle_out = !e->toggle_out;

	int i;
	for( i = -8; i < cep; i++ )
	{
		TIM1->CNT = 0;
	}

	if( cep == 0 )
	{
		TIM1->CNT = 0;
		TIM1->CNT = 0;
		TIM1->CNT = 0;
		TIM1->CNT = 0;

		if( ist->setup_request )
		{
		TIM1->CNT = 0;
		TIM1->CNT = 0;
		TIM1->CNT = 0;
		TIM1->CNT = 0;
		TIM1->CNT = 0;
		TIM1->CNT = 0;
			struct usb_urb * s = __builtin_assume_aligned( (struct usb_urb *)(data+1), 4 );

			uint32_t wvi = s->lValueLSBIndexMSB;
			//Send just a data packet.
			e->count_in = 0;
			e->count_out = 0;
			e->opaque = 0;
			e->is_descriptor = 0;
			ist->setup_request = 0;
			ist->control_max_len = 0;

			if( s->wRequestTypeLSBRequestMSB == 0x01a1 )
			{
				// Class read request.
				uint32_t wlen = s->wLength;
				if( wlen > sizeof(scratchpad) ) wlen = sizeof(scratchpad);
				// The host wants to read back from us.
				ist->control_max_len = wlen;
				e->opaque = 1;
			}
			if( s->wRequestTypeLSBRequestMSB == 0x0921 )
			{
				// Class request (Will be writing)  This is hid_send_feature_report
				ist->control_max_len = sizeof( scratchpad );
				e->opaque = 0xff;
			}
			else if( (s->wRequestTypeLSBRequestMSB & 0xff80) == 0x0680 )
			{
				int i;
				const struct descriptor_list_struct * dl;
				for( i = 0; i < DESCRIPTOR_LIST_ENTRIES; i++ )
				{
					dl = &descriptor_list[i];
					if( dl->lIndexValue == wvi )
						break;
				}

				if( i == DESCRIPTOR_LIST_ENTRIES )
				{
					//??? Somehow fail?  Is 'nak' the right thing? I don't know what to do here.
					goto just_ack;
				}

				// Send back descriptor.
				e->opaque = i;
				e->is_descriptor = 1;
				uint16_t swLen = s->wLength;
				uint16_t elLen = dl->length;
				ist->control_max_len = (swLen < elLen)?swLen:elLen;
			}
			else if( s->wRequestTypeLSBRequestMSB == 0x0500 )
			{
				//Set address.
				ist->my_address = wvi;
			}
		}
		else
		{

		TIM1->CNT = 0;
			// Continuing data.
			if( e->opaque == 0xff  )
			{
		TIM1->CNT = 0;
				uint8_t * start = &scratchpad[e->count_out];
				int l = length-3;
				int i;
				for( i = 0; i < l; i++ )
					start[i] = data[i+1];//((intptr_t)data)>>(i*8);
				e->count_out += l;
			}
			// Allow user code to receive data.
		}
	}
just_ack:
	{
		//Got the right data.  Acknowledge.
		usb_send_data( 0, 0, 2, 0xD2 ); // Send ACK
	}
	return;
}

void usb_pid_handle_ack( uint32_t dummy, uint8_t * data )
{
	struct rv003usb_internal * ist = &rv003usb_internal_data;
	struct usb_endpoint * e = &ist->eps[ist->current_endpoint];
	e->toggle_in = !e->toggle_in;
	e->count_in++;
	return;
}



