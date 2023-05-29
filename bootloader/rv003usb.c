// Could be defined here, or in the processor defines.
#define SYSTEM_CORE_CLOCK 48000000
#define SYSTICK_USE_HCLK

#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>

#define INSTANCE_DESCRIPTORS
#include "rv003usb.h"

struct rv003usb_internal rv003usb_internal_data;

void SystemInit48HSIUNSAFE( void );

int main()
{
	SystemInit48HSIUNSAFE();
	// This comes in hot.  Reasonable chance clocks won't be settled.

	// Enable GPIOs, TIMERs
	RCC->APB2PCENR = RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM1 | RCC_APB2Periph_GPIOA  | RCC_APB2Periph_AFIO | RCC_APB2Periph_TIM1;

// To use time debugging, enable thsi here, and DEBUG_TIMING in the .S
// You must update in tandem
//#define DEBUG_TIMING

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
void usb_pid_handle_setup( uint32_t this_token, uint8_t * data )
{
	struct rv003usb_internal * ist = &rv003usb_internal_data;
	uint8_t addr = (this_token>>8) & 0x7f;
	uint8_t endp = (this_token>>15) & 0xf;

	if( endp >= ENDPOINTS ) goto end;
	if( addr != 0 && addr != ist->my_address ) goto end;

	ist->current_endpoint = endp;
	struct usb_endpoint * e = &ist->eps[endp];
	e->toggle_out = 0;
	e->toggle_in = 1;
	e->count_in = 0;
	ist->setup_request = 1;
end:
	return;
}

void usb_pid_handle_in( uint32_t this_token, uint8_t * data, uint32_t last_32_bit, int crc_out )
{
	struct rv003usb_internal * ist = &rv003usb_internal_data;
	uint8_t addr = (this_token>>8) & 0x7f;
	uint8_t endp = (this_token>>15) & 0xf;
	if( endp )
		goto send_nada;
	//If we get an "in" token, we have to strike any accept buffers.

	if( addr != 0 && addr != ist->my_address ) return;
	struct usb_endpoint * e = &ist->eps[0];
	int tosend = 0;
	uint8_t * sendnow = data;
	uint8_t sendtok = e->toggle_in?0b01001011:0b11000011;
	
	// Handle IN (sending data back to PC)
	switch( endp )
	{
		case 0: // control endpoint.
			if( e->is_descriptor )
			{
				const struct descriptor_list_struct * dl = &descriptor_list[e->opaque];
				int offset = (e->count_in)<<3;
				tosend = ist->control_max_len - offset;
				if( tosend > ENDPOINT0_SIZE ) tosend = ENDPOINT0_SIZE;
				if( tosend < 0 ) tosend = 0;
				sendnow = ((uint8_t*)dl->addr) + offset;
			}
			else
			{
				// I guess we let the user handle this one.
			}
		default:
			break; //no data
	}
	

	if( !tosend )
	{
		goto send_nada;
	}
	else
	{
		usb_send_data( sendnow, tosend, 0, sendtok );
	}
	return;
send_nada:
	sendnow[0] = 0;
	sendnow[1] = 0; //CRC = 0
	usb_send_data( sendnow, 2, 2, sendtok );  //DATA = 0, 0 CRC.
}

void usb_pid_handle_out( uint32_t this_token, uint8_t * data )
{
	struct rv003usb_internal * ist = &rv003usb_internal_data;

	//We need to handle this here because we could have an interrupt in the middle of a control or bulk transfer.
	//This will correctly swap back the endpoint.
	uint8_t addr = (this_token>>8) & 0x7f;
	uint8_t endp = (this_token>>15) & 0xf;
	if( endp >= ENDPOINTS ) return;
	if( addr != 0 && addr != ist->my_address ) return;
	ist->current_endpoint = endp;
	struct usb_endpoint * e = &ist->eps[endp];
	e->count_in = 0;  //Cancel any in transaction
}

void usb_pid_handle_data( uint32_t this_token, uint8_t * data, uint32_t which_data, uint32_t length )
{
	//Received data from host.
	struct rv003usb_internal * ist = &rv003usb_internal_data;
	struct usb_endpoint * e = &ist->eps[ist->current_endpoint];

	if( e == 0 ) return;

	if( e->toggle_out != which_data )
	{
		goto just_ack;
	}

	e->toggle_out = !e->toggle_out;

	if( ist->setup_request )
	{
		ist->setup_request = 0;
		struct usb_urb * s = (struct usb_urb *)(data);

		//Send just a data packet.
		e->count_in = 0;
		e->is_descriptor = 0;
		e->opaque = 0;

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

				// Send back descriptor.
				e->opaque = i;
				e->is_descriptor = 1;
				uint16_t swLen = s->wLength;
				uint16_t elLen = dl->length;
				ist->control_max_len = (swLen < elLen)?swLen:elLen;
			}
		}
		else if( s->bmRequestType == 0x00 )
		{
			if( s->bRequest == 0x05 ) //Set address.
			{
				ist->my_address = s->wValue;
			}
			//if( s->bRequest == 0x09 ) //Set configuration.
			//{
			//}
		}
	}
	else
	{
		// Allow user code to receive data.
	}

just_ack:
	{
		//Got the right data.  Acknowledge.
		usb_send_data( 0, 0, 2, 0xD2 ); // Send ACK
	}
	return;
}

void usb_pid_handle_ack( uint32_t this_token, uint8_t * data )
{
	struct rv003usb_internal * ist = &rv003usb_internal_data;
	struct usb_endpoint * e = &ist->eps[ist->current_endpoint];
	e->toggle_in = !e->toggle_in;
	e->count_in++;
	return;
}


void InterruptVector()         __attribute__((naked)) __attribute((section(".init"))) __attribute((weak,alias("InterruptVectorDefault")));
void InterruptVectorDefault()  __attribute__((naked)) __attribute((section(".init")));
//void EXTI7_0_IRQHandler( void )          __attribute__((section(".text.vector_handler"))) __attribute((weak,alias("DefaultIRQHandler"))) __attribute__((used));

void InterruptVectorDefault()
{
	asm volatile( "\n\
	.align  2\n\
	.option   push;\n\
	.option   norvc;\n\
	j handle_reset\n\
.option push\n\
.option norelax\n\
handle_reset:\n\
	la gp, __global_pointer$\n\
.option pop\n\
	la sp, _eusrstack\n"
".option pop\n"
#if __GNUC__ > 10
".option arch, +zicsr\n"
#endif
"\n\
	li a0, 0x80\n\
	csrw mstatus, a0\n\
	li a3, 0x0\n\
	csrw 0x804, a3\n\
	li a0, 3\n\
	csrw mtvec, a0\n\
	la a0, _sbss\n\
	la a1, _ebss\n\
	li a2, 0\n\
	bge a0, a1, 2f\n\
1:	sw a2, 0(a0)\n\
	addi a0, a0, 4\n\
	blt a0, a1, 1b\n\
2:\n\
");
asm volatile( "\
	csrw mepc, %[main]\n\
	mret\n\
	.balign 16\n\
	.word   EXTI7_0_IRQHandler         /* EXTI Line 7..0 */                 \n\
" : : [main]"r"(main) );
}


void SystemInit48HSIUNSAFE( void )
{
	// Values lifted from the EVT.  There is little to no documentation on what this does.
	RCC->CFGR0 = RCC_HPRE_DIV1 | RCC_PLLSRC_HSI_Mul2;      // PLLCLK = HSI * 2 = 48 MHz; HCLK = SYSCLK = APB1
	RCC->CTLR  = RCC_HSION | RCC_PLLON | ((HSITRIM) << 3); // Use HSI, but enable PLL.
	FLASH->ACTLR = FLASH_ACTLR_LATENCY_1;                  // 1 Cycle Latency
	RCC->INTR  = 0x009F0000;                               // Clear PLL, CSSC, HSE, HSI and LSI ready flags.

	// From SetSysClockTo_48MHZ_HSI
//	while((RCC->CTLR & RCC_PLLRDY) == 0);                                      // Wait till PLL is ready
	RCC->CFGR0 = ( RCC->CFGR0 & ((uint32_t)~(RCC_SW))) | (uint32_t)RCC_SW_PLL; // Select PLL as system clock source
//	while ((RCC->CFGR0 & (uint32_t)RCC_SWS) != (uint32_t)0x08);                // Wait till PLL is used as system clock source
}


