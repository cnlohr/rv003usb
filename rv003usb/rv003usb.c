#include <stdint.h>

#define INSTANCE_DESCRIPTORS 1

#include "rv003usb.h"
#include "usb_config.h"

#if defined(USE_RV003_C) && USE_RV003_C

#include "ch32v003fun.h"

#define ENDPOINT0_SIZE 8 //Fixed for USB 1.1, Low Speed.

struct rv003usb_internal rv003usb_internal_data;

#define LOCAL_CONCAT(A, B) A##B
#define LOCAL_EXP(A, B) LOCAL_CONCAT(A,B)

void usb_setup()
{
	rv003usb_internal_data.se0_windup = 0;

	// Enable GPIOs, TIMERs
	RCC->APB2PCENR |= LOCAL_EXP( RCC_APB2Periph_GPIO, USB_PORT )| RCC_APB2Periph_AFIO;

	// GPIO D3 for input pin change.
	LOCAL_EXP( GPIO, USB_PORT )->CFGLR = 
		( LOCAL_EXP( GPIO, USB_PORT )->CFGLR & 
			(~( ( ( 0xf << (USB_DM*4)) | ( 0xf << (USB_DPU*4)) | ( 0xf << (USB_DP*4)) ) )) )
		 |
		(GPIO_Speed_In | GPIO_CNF_IN_PUPD)<<(4*USB_DM) |  //PD3 = GPIOD IN
		(GPIO_Speed_In | GPIO_CNF_IN_PUPD)<<(4*USB_DP) |  //PD4 = GPIOD IN
		(GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*USB_DPU);

	int port_id = (((intptr_t)LOCAL_EXP( GPIO, USB_PORT )-(intptr_t)GPIOA)>>10);
	// Configure the IO as an interrupt.
	AFIO->EXTICR = (port_id)<<(USB_DP*2); //PORTD.3 (3 out front says PORTD, 3 in back says 3)
	EXTI->INTENR = 1<<USB_DP; // Enable EXT3
	EXTI->FTENR = 1<<USB_DP;  // Rising edge trigger

	// This drive pull-up high, which will tell the host that we are going on-bus.
	GPIOD->BSHR = 1<<USB_DPU;

	// enable interrupt
	NVIC_EnableIRQ( EXTI7_0_IRQn );
}

void usb_hande_interrupt_in( struct usb_endpoint * e, uint8_t * scratchpad, uint32_t sendtok ) __attribute((weak,alias("usb_hande_interrupt_in_default")));
void usb_handle_control_in( struct usb_endpoint * e, uint8_t * scratchpad, uint32_t sendtok )  __attribute((weak,alias("usb_handle_control_in_default")));
void usb_handle_control_out( struct usb_endpoint * e, uint8_t * data, int len )  __attribute((weak,alias("usb_handle_control_out_default")));
void usb_handle_control_out_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )  __attribute((weak,alias("usb_handle_control_out_start_default")));
void usb_handle_control_in_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )  __attribute((weak,alias("usb_handle_control_in_start_default")));

void usb_hande_interrupt_in_default( struct usb_endpoint * e, uint8_t * scratchpad, uint32_t sendtok )
{
	usb_send_nak( sendtok );
}

void usb_handle_control_in_default( struct usb_endpoint * e, uint8_t * scratchpad, uint32_t sendtok )
{
	usb_send_nak( sendtok );
}

void usb_handle_control_out_default( struct usb_endpoint * e, uint8_t * data, int len )
{
	e->count++;
}

void usb_handle_control_out_start_default( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB ) { }
void usb_handle_control_in_start_default( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB ) { }



void usb_pid_handle_in( uint32_t addr, uint8_t * data, uint32_t endp, uint32_t unused, struct rv003usb_internal * ist )
{
	ist->current_endpoint = endp;
	struct usb_endpoint * e = &ist->eps[endp];

	int tosend = 0;
	uint8_t * sendnow = data-1;
	int sendtok = e->toggle_in?0b01001011:0b11000011;

	if( endp )
	{
		usb_hande_interrupt_in( e, sendnow, sendtok );
		return;
	}
	if( !e->is_descriptor )
	{
		usb_handle_control_in( e, sendnow, sendtok );
		return;
	}

	tosend = 0;
	sendnow = data-1;

	// Handle IN (sending data back to PC)
	// Do this down here.
	// We do this because we are required to have an in-endpoint.  We don't
	// have to do anything with it, though.
	uint8_t * tsend = 0;

	const struct descriptor_list_struct * dl = &descriptor_list[e->opaque];
	tsend = ((uint8_t*)dl->addr);

	int offset = (e->count)<<3;
	tosend = e->max_len - offset;
	if( tosend > ENDPOINT0_SIZE ) tosend = ENDPOINT0_SIZE;
	sendnow = tsend + offset;

	if( tosend < 0 ) tosend = 0;

	if( !tosend || !sendnow )
	{
		usb_send_nak( sendtok );
	}
	else
	{
		usb_send_data( sendnow, tosend, 0, sendtok );
	}
	return;
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
	int cep = ist->current_endpoint;
	struct usb_endpoint * e = &ist->eps[cep];

	// Alrady received this packet.
	if( e->toggle_out != which_data )
	{
		goto just_ack;
	}

	e->toggle_out = !e->toggle_out;

	if( cep == 0 )
	{
		if( ist->setup_request )
		{
			struct usb_urb * s = __builtin_assume_aligned( (struct usb_urb *)(data+1), 4 );

			uint32_t wvi = s->lValueLSBIndexMSB;
			//Send just a data packet.
			e->count = 0;
			e->opaque = 0;
			e->is_descriptor = 0;
			ist->setup_request = 0;
			e->max_len = 0;

			if( s->wRequestTypeLSBRequestMSB == 0x01a1 )
			{
				// Class read request.
				// The host wants to read back from us. hid_get_feature_report
				usb_handle_control_in_start( e, s->wLength, wvi );
				e->opaque = 1;
			}
			else if( s->wRequestTypeLSBRequestMSB == 0x0921 )
			{
				// Class request (Will be writing)  This is hid_send_feature_report
				usb_handle_control_out_start( e, s->wLength, wvi );
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
				e->max_len = (swLen < elLen)?swLen:elLen;
			}
			else if( s->wRequestTypeLSBRequestMSB == 0x0500 )
			{
				//Set address.
				ist->my_address = wvi;
			}
		}
		else
		{
			// Continuing data.
			if( e->opaque == 0xff  )
			{
				usb_handle_control_out( e, data + 1, length - 3 );
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


#if defined( REALLY_TINY_COMP_FLASH ) && REALLY_TINY_COMP_FLASH

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
}
#endif


#endif


