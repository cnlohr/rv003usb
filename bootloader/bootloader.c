// Could be defined here, or in the processor defines.
#define SYSTEM_CORE_CLOCK 48000000
#define SYSTICK_USE_HCLK

#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>

#define INSTANCE_DESCRIPTORS
#include "rv003usb.h"

// To use time debugging, enable thsi here, and DEBUG_TIMING in the .S
// You must update in tandem
//#define DEBUG_TIMING

// If you don't want to automatically boot into the application, set
// this flag:
//#define DISABLE_BOOTLOAD

#define SCRATCHPAD_SIZE 128
extern volatile int32_t runwordpad;
extern uint8_t scratchpad[SCRATCHPAD_SIZE];

struct rv003usb_internal rv003usb_internal_data;

// This is the data actually required for USB.
uint8_t data_receptive;

void SystemInit48HSIUNSAFE( void );

int main()
{
	SysTick->CTLR = 5;

	// Enable GPIOs, TIMERs
	RCC->APB2PCENR = RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM1 | RCC_APB2Periph_GPIOA  | RCC_APB2Periph_AFIO | RCC_APB2Periph_TIM1;

#ifdef DEBUG_TIMING
	// GPIO C0 Push-Pull
	GPIOC->CFGLR = (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*0) |
	               (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF)<<(4*3) | // PC3 = T1C3
	               (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF)<<(4*4) | // PC4 = T1C4
	               (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*2);
#endif

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

	// GPIO D3 for input pin change.
	GPIOD->CFGLR =
		(GPIO_CNF_IN_PUPD)<<(4*1) |  // Keep SWIO enabled.
//		(GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*DEBUG_PIN) |
		(GPIO_Speed_In | GPIO_CNF_IN_PUPD)<<(4*USB_DM) |  //PD3 = GPIOD IN
		(GPIO_Speed_In | GPIO_CNF_IN_PUPD)<<(4*USB_DP) |  //PD4 = GPIOD IN
		(GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*USB_DPU);

	// Configure the IO as an interrupt.
	AFIO->EXTICR = 3<<(USB_DP*2); //PORTD.3 (3 out front says PORTD, 3 in back says 3)
	EXTI->INTENR = 1<<USB_DP; // Enable EXT3
	EXTI->FTENR = 1<<USB_DP;  // Rising edge trigger

	// This drive GPIO5 high, which will tell the host that we are going on-bus.
	GPIOD->BSHR = 1<<USB_DPU;

	// enable interrupt
	NVIC_EnableIRQ( EXTI7_0_IRQn );

	// Wait ~5 seconds.
	int32_t localpad = -0xf00000; // Careful: Constant works out to a single lbu instruction.
	while(1)
	{
		if( localpad < 0 )
		{
			if( ++localpad == 0 )
			{
				// Boot to user program.
#ifndef DISABLE_BOOTLOAD
				FLASH->BOOT_MODEKEYR = FLASH_KEY1;
				FLASH->BOOT_MODEKEYR = FLASH_KEY2;
				FLASH->STATR = 0; // 1<<14 is zero, so, boot user code.
				FLASH->CTLR = CR_LOCK_Set;
				PFIC->SCTLR = 1<<31;
#endif
			}
		}
		if( localpad > 0 )
		{
			if( --localpad == 0 )
			{
				/* Scratchpad strucure:
					4-bytes:		LONG( 0x000000aa )
						... code (this is executed) (120 bytes)
					4-bytes:        LONG( 0x1234abcd )

					After the scratchpad is the runpad, its structure is:
					4-bytes:   int32_t  if negative, how long to go before bootloading.  If 0, do nothing.  If positive, execute.
						... 1kB of totally free space.
				*/
				typedef void (*setype)( uint32_t *, volatile int32_t * );
				setype scratchexec = (setype)(scratchpad+4);
				scratchexec( (uint32_t*)&scratchpad[0], &runwordpad );
			}
		}

		uint32_t commandpad = runwordpad;
		if( commandpad )
		{
			localpad = commandpad-1;
			runwordpad = 0;
		}
	}
}

#define ENDPOINT0_SIZE 8 //Fixed for USB 1.1, Low Speed.

void usb_pid_handle_in( uint32_t addr, uint8_t * data, uint32_t endp, uint32_t unused, struct rv003usb_internal * ist )
{
	ist->current_endpoint = endp;
	struct usb_endpoint * e = &ist->eps[endp];

	int tosend = 0;
	uint8_t * sendnow = data-1;
	uint8_t sendtok = e->toggle_in?0b01001011:0b11000011;
	
	// Handle IN (sending data back to PC)
	// Do this down here.
	// We do this because we are required to have an in-endpoint.  We don't
	// have to do anything with it, though.
	if( endp )
	{
		usb_send_nak( sendtok );
		return;
	}

	uint8_t * tsend = 0;

	if( e->is_descriptor )
	{
		const struct descriptor_list_struct * dl = &descriptor_list[e->opaque];
		tsend = ((uint8_t*)dl->addr);
	}
	else if( e->opaque == 1 )
	{
		tsend = scratchpad;
	}

	int offset = (e->count)<<3;
	tosend = e->max_len - offset;
	if( tosend > ENDPOINT0_SIZE ) tosend = ENDPOINT0_SIZE;
	if( tosend < 0 ) tosend = 0;
	sendnow = tsend + offset;

	if( !tosend || !tsend )
	{
		usb_send_nak( sendtok );
	}
	else
	{
		usb_send_data( sendnow, tosend, 0, sendtok );
	}
	return;
}

void usb_pid_handle_data( uint32_t this_token, uint8_t * data, uint32_t which_data, uint32_t length, struct rv003usb_internal * ist )
{
	//Received data from host.
	int cep = ist->current_endpoint;
	struct usb_endpoint * e = &ist->eps[cep];

	// Alrady received this packet.
	if( e->toggle_out != which_data )
	{
		goto just_ack;
	}
	// XXX NOTE:  		e->toggle_out = !e->toggle_out; should be here.
	if( cep == 0 )
	{
		e->toggle_out = !e->toggle_out;
		if( ist->setup_request )
		{
			struct usb_urb * s = __builtin_assume_aligned( (struct usb_urb *)(data+1), 4 );

			uint32_t wvi = s->lValueLSBIndexMSB;
			//Send just a data packet.
			e->count = 0;
			e->is_descriptor = 0;
			ist->setup_request = 0;
			e->max_len = 0;

			if( s->wRequestTypeLSBRequestMSB == 0x01a1 )
			{
				// Class read request.
				uint32_t wlen = s->wLength;
				if( wlen > sizeof(scratchpad) ) wlen = SCRATCHPAD_SIZE;
				// The host wants to read back from us.
				e->max_len = wlen;
				e->opaque = 1;
			}
			else if( s->wRequestTypeLSBRequestMSB == 0x0921 )
			{
				// Class write request.
				e->max_len = SCRATCHPAD_SIZE;
				runwordpad = 1; //request stoppage.
				e->opaque = 2;
			}
			else if( (s->wRequestTypeLSBRequestMSB & 0xff80) == 0x0680 )
			{
				int i;
				const struct descriptor_list_struct * dl;
				for( i = 0; i < DESCRIPTOR_LIST_ENTRIES; i++ )
				{
					dl = &descriptor_list[i];
					if( dl->lIndexValue == wvi )
					{
						// Send back descriptor.
						e->opaque = i;
						e->is_descriptor = 1;
						uint16_t swLen = s->wLength;
						uint16_t elLen = dl->length;
						e->max_len = (swLen < elLen)?swLen:elLen;
						break;
					}
				}

				goto just_ack;
			}
			else if( s->wRequestTypeLSBRequestMSB == 0x0500 )
			{
				//Set address.
				ist->my_address = wvi;
				e->opaque = 0;
			}
			else
			{
				e->opaque = 0;
			}
		}
		else
		{
			// Continuing data.
			if( e->opaque == 2 )
			{
				uint8_t * start = &scratchpad[e->count<<3];
				int l = length-3;
				int i;
				for( i = 0; i < l; i++ )
					start[i] = data[i+1];//((intptr_t)data)>>(i*8);
				e->count ++;

				if( e->count >= SCRATCHPAD_SIZE )
				{
					// If the last 4 bytes are 0x1234abcd, then we can go!
					uint32_t * last4 = (uint32_t*)(start + 4);					
					if( *last4 == 0x1234abcd )
					{
						*last4 = 0;
						runwordpad = 0x200; // Request exectution
					}
					e->opaque = 0;
				}
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




