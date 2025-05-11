#include <stdint.h>

#define INSTANCE_DESCRIPTORS 1

#include "rv003usb.h"
#include "usb_config.h"

#if !defined(RV003USB_CUSTOM_C) || RV003USB_CUSTOM_C == 0

#include "ch32fun.h"

#if RV003USB_USB_TERMINAL
#if FUNCONF_USE_DEBUGPRINTF
#include "../lib/swio_self.h" // Needed for unlocking DM
#else 
#define RV003USB_USB_TERMINAL 0
#endif
#endif

#define ENDPOINT0_SIZE 8 //Fixed for USB 1.1, Low Speed.

#if RV003USB_EVENT_DEBUGGING

#define NUMUEVENTS 32
uint32_t events[4*NUMUEVENTS];
volatile uint8_t eventhead, eventtail;
void LogUEvent( uint32_t a, uint32_t b, uint32_t c, uint32_t d )
{
	int event = eventhead;
	uint32_t * e = &events[event*4];
	e[0] = a;
	e[1] = b;
	e[2] = c;
	e[3] = d;
	eventhead = (event + 1) & (NUMUEVENTS-1);
}

uint32_t * GetUEvent()
{
	int event = eventtail;
	if( eventhead == event ) return 0;
	eventtail = ( event + 1 ) & (NUMUEVENTS-1);
	return &events[event*4];
}

#endif



struct rv003usb_internal rv003usb_internal_data;

#define LOCAL_CONCAT(A, B) A##B
#define LOCAL_EXP(A, B) LOCAL_CONCAT(A,B)

void usb_setup()
{
	rv003usb_internal_data.se0_windup = 0;

	// Enable GPIOs, TIMERs
	RCC->APB2PCENR |= LOCAL_EXP( RCC_APB2Periph_GPIO, USB_PORT ) | RCC_APB2Periph_AFIO;

#if defined( RV003USB_DEBUG_TIMING ) && RV003USB_DEBUG_TIMING
	{
		RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM1;

		GPIOC->CFGLR = (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*0) |
			           (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF)<<(4*3) | // PC3 = T1C3
			           (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF)<<(4*4) |
			           (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*2);

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

	// GPIO Setup
	LOCAL_EXP( GPIO, USB_PORT )->CFGLR = 
		( LOCAL_EXP( GPIO, USB_PORT )->CFGLR & 
			(~( ( ( 0xf << (USB_PIN_DP*4)) | ( 0xf << (USB_PIN_DM*4)) 
#ifdef USB_PIN_DPU
				| ( 0xf << (USB_PIN_DPU*4)) 
#endif
			) )) )
		 |
#ifdef USB_PIN_DPU
		(GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*USB_PIN_DPU) |
#endif
		(GPIO_Speed_In | GPIO_CNF_IN_PUPD)<<(4*USB_PIN_DP) | 
		(GPIO_Speed_In | GPIO_CNF_IN_PUPD)<<(4*USB_PIN_DM);

	// Configure USB_PIN_DM (D-) as an interrupt on falling edge.
	AFIO->EXTICR = LOCAL_EXP(GPIO_PortSourceGPIO,USB_PORT)<<(USB_PIN_DM*2); // Configure EXTI interrupt for USB_PIN_DM
	EXTI->INTENR = 1<<USB_PIN_DM; // Enable EXTI interrupt
	EXTI->FTENR = 1<<USB_PIN_DM;  // Enable falling edge trigger for USB_PIN_DM (D-)

#ifdef USB_PIN_DPU
	// This drives USB_PIN_DPU (D- Pull-Up) high, which will tell the host that we are going on-bus.
	LOCAL_EXP(GPIO,USB_PORT)->BSHR = 1<<USB_PIN_DPU;
#endif

	// enable interrupt
	NVIC_EnableIRQ( EXTI7_0_IRQn );
}


void usb_pid_handle_in( uint32_t addr, uint8_t * data, uint32_t endp, uint32_t unused, struct rv003usb_internal * ist )
{
	ist->current_endpoint = endp;
	struct usb_endpoint * e = &ist->eps[endp];

	int tosend = 0;
	uint8_t * sendnow;
	int sendtok = e->toggle_in?0b01001011:0b11000011;



#if RV003USB_USE_REBOOT_FEATURE_REPORT
	if( ist->reboot_armed == 2 )
	{
		usb_send_empty( sendtok );

		// Initiate boot into bootloader
		FLASH->BOOT_MODEKEYR = FLASH_KEY1;
		FLASH->BOOT_MODEKEYR = FLASH_KEY2;
		FLASH->STATR = 1<<14; // 1<<14 is zero, so, boot bootloader code. Unset for user code.
		FLASH->CTLR = CR_LOCK_Set;
		RCC->RSTSCKR |= 0x1000000;
		PFIC->SCTLR = 1<<31;
	}
#endif

#if RV003USB_HANDLE_IN_REQUEST
	if( e->custom || endp )
	{
		// Can reuse data-stack as scratchpad.
		sendnow = __builtin_assume_aligned( data, 4 );
		usb_handle_user_in_request( e, sendnow, endp, sendtok, ist );
		return;
	}
#endif

	// Handle IN (sending data back to PC)
	// Do this down here.
	// We do this because we are required to have an in-endpoint.  We don't
	// have to do anything with it, though.
#if RV003USB_USB_TERMINAL
	if( e->opaque == (uint8_t *)DMDATA0 )
	{
		usb_send_data( (void *)DMDATA0, ENDPOINT0_SIZE, 0, sendtok );
		*DMDATA0 = 0;
		*DMDATA1 = 0;
		e->opaque = 0;
		return;
	}
#endif


	uint8_t * tsend = e->opaque;

	int offset = (e->count)<<3;
	tosend = (int)e->max_len - offset;
	if( tosend > ENDPOINT0_SIZE ) tosend = ENDPOINT0_SIZE;
	sendnow = tsend + offset;
	if( tosend <= 0 )
	{
		usb_send_empty( sendtok );
	}
	else
	{
		usb_send_data( sendnow, tosend, 0, sendtok );
	}
}

void usb_pid_handle_out( uint32_t addr, uint8_t * data, uint32_t endp, uint32_t unused, struct rv003usb_internal * ist )
{
	ist->current_endpoint = endp;

	//We need to handle this here because we could have an interrupt in the middle of a control or big transfer.
	//This will correctly swap back the endpoint.
}

void usb_pid_handle_data( uint32_t this_token, uint8_t * data, uint32_t which_data, uint32_t length, struct rv003usb_internal * ist )
{
	//Received data from host.
	int epno = ist->current_endpoint;
	struct usb_endpoint * e = &ist->eps[epno];

	length -= 3;
	uint8_t * data_in = __builtin_assume_aligned( data, 4 );

	// Already received this packet.
	if( e->toggle_out != which_data )
	{
		goto just_ack;
	}

	e->toggle_out = !e->toggle_out;


#if RV003USB_HANDLE_USER_DATA || RV003USB_USE_REBOOT_FEATURE_REPORT || RV003USB_USB_TERMINAL
	if( epno || ( !ist->setup_request && length > 3 )  )
	{
#if RV003USB_USE_REBOOT_FEATURE_REPORT
		if( ist->reboot_armed )
		{
			uint32_t * base = __builtin_assume_aligned( data_in, 4 );
			if( epno == 0 && base[0] == 0xaa3412fd && (base[1] & 0x00ffffff) == 0x00ddccbb )
			{
				e->count = 7;
				ist->reboot_armed = 2;
				goto just_ack;
			}
			else
			{
				ist->reboot_armed = 0;
			}
		}
#endif
#if RV003USB_USB_TERMINAL
		if( epno == 0 && data_in[0] == 0xfd )
		{
			uint32_t *base = __builtin_assume_aligned( data_in, 4 );

			*DMDATA0 = base[0];
			*DMDATA1 = base[1];
			*DMDATA0 &= ~( 0xFF );

			int i;
			for( i = 1; i < 8; i++ )
			{
				if( data_in[i] == 0 )
				{
					*DMDATA0 |= i + 3;
					break;
				}
			}

			goto just_ack;
		}
#endif

#if RV003USB_HANDLE_USER_DATA
		usb_handle_user_data( e, epno, data_in, length, ist );
#endif
#if RV003USB_USER_DATA_HANDLES_TOKEN
		return;
#endif
	}
	else
#endif

	if( ist->setup_request )
	{
		// For receiving CONTROL-OUT messages into RAM.  NOTE: MUST be ALIGNED to 4, and be allocated round_up( payload, 8 ) + 4
		// opaque points to [length received, but uninitialized before complete][data...]
#if RV003USB_SUPPORT_CONTROL_OUT
		if( ist->setup_request == 2 )
		{
			// This mode is where we record control-in data into a pointer and mark it as 
			int offset = e->count << 3;
			uint32_t * base = __builtin_assume_aligned( e->opaque, 4 );
			uint32_t * dout = __builtin_assume_aligned( ((uint8_t*)base) + offset + 4, 4 );
			uint32_t * din = __builtin_assume_aligned( data_in, 4 );
			if( offset < e->max_len )
			{
				dout[0] = din[0];
				dout[1] = din[1];
				e->count++;
				if( offset + 8 >= e->max_len )
				{
					base[0] = e->max_len;
				}
			}
			goto just_ack;
		}
#endif

		struct usb_urb * s = __builtin_assume_aligned( (struct usb_urb *)(data_in), 4 );

		uint32_t wvi = s->lValueLSBIndexMSB;
		uint32_t wLength = s->wLength;
		//Send just a data packet.
		e->count = 0;
		e->opaque = 0;
		e->custom = 0;
		e->max_len = 0;
		ist->setup_request = 0;

		//int bRequest = s->wRequestTypeLSBRequestMSB >> 8;

		// We shift down because we don't care if USB_RECIP_INTERFACE is set or not.
		// Otherwise we have to write extra code to handle each case if it's set or
		// not set, but in general, there's never a situation where we really care.
		uint32_t reqShl = s->wRequestTypeLSBRequestMSB >> 1;

		//LogUEvent( 0, s->wRequestTypeLSBRequestMSB, wvi, s->wLength );

		if( reqShl == (0x0921>>1) )
		{
			// Class request (Will be writing)  This is hid_send_feature_report
#if RV003USB_USE_REBOOT_FEATURE_REPORT
			if( wvi == 0x000003fd ) ist->reboot_armed = 1;
#endif
#if RV003USB_HID_FEATURES
			usb_handle_hid_set_report_start( e, wLength, wvi );
#endif
		}
		else
#if RV003USB_HID_FEATURES || RV003USB_USB_TERMINAL
		if( reqShl == (0x01a1>>1) )
		{
			// Class read request.
			// The host wants to read back from us. hid_get_feature_report
#if RV003USB_HID_FEATURES
			usb_handle_hid_get_report_start( e, wLength, wvi );
#endif
#if RV003USB_USB_TERMINAL
			if( ( wvi & 0xff ) == 0xfd )
			{
				e->opaque = 0;	// If it's 0xFD feature discard anything that could be set in usb_handle_hid_get_report_start
				e->max_len = 1; // If 0 - terminal is sent to the stratosphere
				if( !*DMSTATUS_SENTINEL )
				{
					if( ( *DMDATA0 & 0x80 ) )
					{
						e->opaque = (uint8_t *)DMDATA0;
						e->max_len = ( *DMDATA0 & 0xf ) - 4;
						// e->max_len = 8;
					}
					else if( ( *DMDATA0 & 0xc0 ) == 0xc0 )
					{
						*DMDATA0 = 0;
					}
				}
				else
				{
					attempt_unlock( 2 );
				}
			}
#endif
		}
		else
#endif
		if( reqShl == (0x0680>>1) ) // GET_DESCRIPTOR = 6 (msb)
		{
			int i;
			const struct descriptor_list_struct * dl;
			for( i = 0; i < DESCRIPTOR_LIST_ENTRIES; i++ )
			{
				dl = &descriptor_list[i];
				if( dl->lIndexValue == wvi )
				{
					e->opaque = (uint8_t*)dl->addr;
					uint16_t swLen = wLength;
					uint16_t elLen = dl->length;
					e->max_len = (swLen < elLen)?swLen:elLen;
				}
			}
#if RV003USB_EVENT_DEBUGGING
			if( !e->max_len ) LogUEvent( 1234, dl->lIndexValue, dl->length, 0 );
#endif
		}
		else if( reqShl == (0x0500>>1) ) // SET_ADDRESS = 0x05
		{
			ist->my_address = wvi;
		}
#if 0
		// These are optional for the most part.
		else if( reqShl == (0x0080>>1) ) // GET_STATUS = 0x00 ,always reply with { 0x00, 0x00 } 
		{
			e->opaque = (uint8_t*)always0;
			e->max_len = wLength;
		}
		else if( reqShl == (0x0a21>>1) ) // GET_INTERFACE = 0x00, always reply with { 0x00 } 
		{
			e->opaque = (uint8_t*)always0;
			e->max_len = wLength;
		}
#endif
		//  You could handle SET_CONFIGURATION == 0x0900 here if you wanted.
		//  Can also handle GET_CONFIGURATION == 0x0880 to which we send back { 0x00 }, or the interface number.  (But no one does this).
		//  You could handle SET_INTERFACE == 0x1101 here if you wanted.
		//   or
		//  USB_REQ_GET_INTERFACE to which we would send 0x00, or the interface #
		else
		{
#if RV003USB_OTHER_CONTROL
			usb_handle_other_control_message( e, s, ist );
#endif
		}
	}
just_ack:
	{
		//Got the right data.  Acknowledge.
		usb_send_data( 0, 0, 2, 0xD2 ); // Send ACK
	}
	return;
}


#if defined( RV003USB_OPTIMIZE_FLASH ) && RV003USB_OPTIMIZE_FLASH

// Do not compile ACK commands.

#else

void usb_pid_handle_ack( uint32_t dummy, uint8_t * data, uint32_t dummy1, uint32_t dummy2, struct rv003usb_internal * ist  )
{
	struct usb_endpoint * e = &ist->eps[ist->current_endpoint];
	e->toggle_in = !e->toggle_in;
	e->count++;
}


//Received a setup for a specific endpoint.
void usb_pid_handle_setup( uint32_t addr, uint8_t * data, uint32_t endp, uint32_t unused, struct rv003usb_internal * ist )
{
	struct usb_endpoint * e = &ist->eps[endp];
	ist->current_endpoint = endp;
	ist->setup_request = 1;
	e->toggle_in = 1;
	e->toggle_out = 0;
	e->count = 0;
	e->opaque = 0;
}
#endif


#endif


