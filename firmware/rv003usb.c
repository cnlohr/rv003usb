// Could be defined here, or in the processor defines.
#define SYSTEM_CORE_CLOCK 48000000
#define SYSTICK_USE_HCLK

#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>

#define INSTANCE_DESCRIPTORS
#include "rv003usb.h"

uint32_t test_memory[2];
uint32_t plen;
int8_t did_get_data;
uint32_t usb_buffer_back[USB_BUFFER_SIZE];

struct rv003usb_internal rv003usb_internal_data;

// This is the data actually required for USB.
uint8_t data_receptive;

void usb_handle_custom_control( uint8_t bmRequestType, uint8_t bRequest, uint16_t wLength,  struct rv003usb_internal * ist )
{
	// Do something.
}

int main()
{
	memset( usb_buffer_back, 0, sizeof( usb_buffer_back ) );
	did_get_data = 0;

	SystemInit48HSI();
	SetupDebugPrintf();
	SETUP_SYSTICK_HCLK

	
	Delay_Ms( 1000 );
//	printf( "......................\n" );

	// Enable GPIOs, DMA and TIMERs
	RCC->AHBPCENR = RCC_AHBPeriph_SRAM | RCC_AHBPeriph_DMA1;
	RCC->APB2PCENR = RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM1 | RCC_APB2Periph_GPIOA  | RCC_APB2Periph_AFIO | RCC_APB2Periph_TIM1;

	// GPIO C0 Push-Pull
	GPIOC->CFGLR = (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*0) |
	               (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF)<<(4*3) | // PC3 = T1C3
	               (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF)<<(4*4) | // PC4 = T1C4
	               (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*2);


	// PC4 is MCO (for watching timing)
	GPIOC->CFGLR &= ~(GPIO_CFGLR_MODE4 | GPIO_CFGLR_CNF4);
	GPIOC->CFGLR |= GPIO_CFGLR_CNF4_1 | GPIO_CFGLR_MODE4_0 | GPIO_CFGLR_MODE4_1;
	RCC->CFGR0 = (RCC->CFGR0 & ~RCC_CFGR0_MCO) | RCC_CFGR0_MCO_SYSCLK;

	{
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

	// Disable fast interrupts. "HPE"
	asm volatile( "addi t1,x0, 0\ncsrrw x0, 0x804, t1\n" : : :  "t1" );

	// enable interrupt
	NVIC_EnableIRQ( EXTI7_0_IRQn );


//	uint8_t my_ep2[8];
//	struct rv003usb_internal * uis = &rv003usb_internal_data;
//	struct usb_endpoint * e2 = &uis->eps[2];

//	e2->ptr_in = my_ep2;
//	e2->place_in = 0;
//	e2->size_in = sizeof( my_ep2 );
	

	while(1)
	{
		//GPIOC->BSHR = 1;   // Set pin high
		//Delay_Ms( 1000 );
		//GPIOC->BSHR = (1<<16); // Set the pin low
		//printf( "hello\n" );
/*
		uint8_t * buffer = (uint8_t*)usb_buffer_back;
		printf( " -- %08lx\n", plen );
		int i;
		for( i = 0; i < 16; i++ )
			printf( "%02x ", buffer[i] );
		Delay_Ms( 100 );
*/
//		printf( "%08lx\n", test_memory[0] );
		Delay_Ms( 100 );

	//	my_ep2[0] = 0x14;
	//	my_ep2[2] = 0x80;
	//	e2->send = 1;

		//Delay_Ms( 1000 );
		//dma_buffer[0] = 0;
		//dma_buffer[1] = 0;
		//printf( "%08lx %08lx\n", test_memory[0], test_memory[1] );
	}
}







#define ENDPOINT0_SIZE 8 //Fixed for USB 1.1, Low Speed.


//Received a setup for a specific endpoint.
void usb_pid_handle_setup( uint32_t this_token, struct rv003usb_internal * ist )
{
	uint8_t addr = (this_token>>8) & 0x7f;
	uint8_t endp = (this_token>>15) & 0xf;

	ist->there_is_a_host = 1;

	if( endp >= ENDPOINTS ) goto end;
	if( addr != 0 && addr != ist->my_address ) goto end;

	struct usb_endpoint * e = ist->ce = &ist->eps[endp];
	e->toggle_out = 0;
	e->toggle_in = 1;
	e->ptr_in = 0;
	e->send = 0;
	ist->setup_request = 1;
end:
	return;
}

void usb_pid_handle_in( uint32_t this_token, struct rv003usb_internal * ist, uint32_t last_32_bit, int crc_out )
{
	uint8_t addr = (this_token>>8) & 0x7f;
	uint8_t endp = (this_token>>15) & 0xf;
//printf( "PID_IN: %02x %02x %08x %08x\n", addr, endp, last_32_bit, crc_out );
	//If we get an "in" token, we have to strike any accept buffers.

	if( endp >= ENDPOINTS ) return;
	if( addr != 0 && addr != ist->my_address ) return;

	struct usb_endpoint * e = ist->ce = &ist->eps[endp];

	e->got_size_out = 0;  //Cancel any out transaction


	int tosend = 0;
	uint8_t sendnow[12];

	sendnow[0] = 0x80;

	if( e->send && e->ptr_in ) 
	{
		tosend = e->size_in - e->place_in;

		if( tosend > 8 ) tosend = 8;
	}

	if( e->toggle_in )
	{
		sendnow[1] = 0b01001011; //DATA1
	}
	else
	{
		sendnow[1] = 0b11000011; //DATA0
	}

	if( tosend == 0 || !e->send || !e->ptr_in || e->ptr_in == EMPTY_SEND_BUFFER )  //Tricky: Empty packet.
	{

		//Tricky: Control messages are not allowed to send NAKs.  We /have/ to send an empty packet for them if no more data is available.
		//With endpoints, proper, it's okay to send NAKs.

		if( endp == 0 )
		{
			sendnow[2] = 0;
			sendnow[3] = 0; //CRC = 0
			usb_send_data( sendnow, 4, 2 );  //Force a CRC
			e->ptr_in = 0;
		}
		else
		{
			sendnow[1] = 0x5a; //Empty data (NAK)
			usb_send_data( sendnow, 2, 2 );
		}
	}
	else
	{
		if( tosend & 1 )
		{
			// Super tricky: If we are odd, it will accidentally
			// skip the second byte.
			memcpy( sendnow+3, e->ptr_in + e->place_in, tosend );
			sendnow[2] = sendnow[1];
		}
		else
		{
			memcpy( sendnow+2, e->ptr_in + e->place_in, tosend );
		}
		usb_send_data( sendnow, tosend+2, 0 );
		e->advance_in = tosend;
	}
}

void usb_pid_handle_out( uint32_t this_token, struct rv003usb_internal * ist )
{
	//We need to handle this here because we could have an interrupt in the middle of a control or bulk transfer.
	//This will correctly swap back the endpoint.
	uint8_t addr = (this_token>>8) & 0x7f;
	uint8_t endp = (this_token>>15) & 0xf;
	if( endp >= ENDPOINTS ) return;
	if( addr != 0 && addr != ist->my_address ) return;
	/*struct usb_endpoint * e = */ist->ce = &ist->eps[endp];
}

void usb_pid_handle_data( uint32_t this_token, struct rv003usb_internal * ist, uint32_t which_data )
{
	//Received data from host.

	struct usb_endpoint * e = ist->ce;

	if( e == 0 ) return;

	if( e->toggle_out != which_data )
	{
		goto just_ack;
	}

	e->toggle_out = !e->toggle_out;

	if( ist->setup_request )
	{
		ist->setup_request = 0;
		struct usb_urb * s = (struct usb_urb *)ist->usb_buffer;

		//Send just a data packet.
		e->ptr_in = EMPTY_SEND_BUFFER;
		e->place_in = 0;
		e->size_in = 0;
		e->send = 1;

		if( s->bmRequestType & 0x80 )
		{
			if( s->bRequest == 0x06 ) //Get Descriptor Request
			{
				int i;
				const struct descriptor_list_struct * dl;
				for( i = 0; i < DESCRIPTOR_LIST_ENTRIES; i++ )
				{
					dl = &descriptor_list[i];
					if( dl->wIndex == s->wIndex && dl->wValue == s->wValue )
						break;
				}

				if( i == DESCRIPTOR_LIST_ENTRIES )
				{
					//??? Somehow fail?  Is 'nak' the right thing? I don't know what to do here.
					goto just_ack;
				}

				e->ptr_in = dl->addr;
				e->size_in = dl->length;
				if( s->wLength < e->size_in ) e->size_in = s->wLength;
			}
			usb_handle_custom_control( s->bmRequestType, s->bRequest, s->wLength, ist );
		}
		else if( s->bmRequestType == 0x00 )
		{
			if( s->bRequest == 0x05 ) //Set address.
			{
				ist->my_address = s->wValue;
			}
			if( s->bRequest == 0x09 ) //Set configuration.
			{
				//s->wValue; has the index.  We don't really care about this.
			}
			usb_handle_custom_control( s->bmRequestType, s->bRequest, s->wLength, ist );
		}
	}
	else if( e->ptr_out )
	{
		//Read into that buffer.
		int acc = ist->packet_size-3;  //packet_size includes CRC and PID, need just data size.
		int place = e->got_size_out;

		if( place + acc > e->max_size_out )
		{
			acc = e->max_size_out - place;
		}

		memcpy( e->ptr_out + e->got_size_out, ist->usb_buffer+1, acc );  //First byte of USB buffer is token.
		e->got_size_out += acc;
		if( e->got_size_out == e->max_size_out && e->transfer_done_ptr ) {
			e->ptr_out = 0;
			*e->transfer_done_ptr = e->got_size_out;
			e->transfer_done_ptr = 0;
		}
	}


just_ack:
	{
		//Got the right data.  Acknowledge.
		uint8_t sendword[2] = { 0x80, 0xD2 };
		usb_send_data( sendword, 2, 2 );
	}
	return;
}

void usb_pid_handle_ack( uint32_t this_token, struct rv003usb_internal * ist )
{
	struct usb_endpoint * e = ist->ce;
	if( !e ) goto term;

	e->toggle_in = !e->toggle_in;
	e->place_in += e->advance_in;
	e->advance_in = 0;
	if( e->place_in == e->size_in )
	{
		e->send = 0;
		if( e->transfer_in_done_ptr ) (*e->transfer_in_done_ptr) = 1;
		e->transfer_in_done_ptr = 0;
	}
term:
	return;
}



