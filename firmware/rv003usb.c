// Could be defined here, or in the processor defines.
#define SYSTEM_CORE_CLOCK 48000000
#define SYSTICK_USE_HCLK

#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

uint32_t test_memory[2];
uint32_t usb_buffer[USB_BUFFER_SIZE];

uint32_t plen;
uint32_t pcrc;

int8_t did_get_data;
uint32_t usb_buffer_back[USB_BUFFER_SIZE];

void rv003usb_handle_packet( uint8_t * buffer, int length, uint32_t crc )
{
	if( did_get_data > 0 ) did_get_data--;
	else
	if( !did_get_data )
	{
		plen = length;
		pcrc = crc;
		memcpy( usb_buffer_back, usb_buffer, sizeof( usb_buffer_back ) );
		did_get_data = -1;
	}
}

int main()
{
	memset( usb_buffer_back, 0, sizeof( usb_buffer_back ) );
	did_get_data = 1;

	SystemInit48HSI();
	SetupDebugPrintf();
	SETUP_SYSTICK_HCLK

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

	while(1)
	{
		//GPIOC->BSHR = 1;   // Set pin high
		//Delay_Ms( 1000 );
		//GPIOC->BSHR = (1<<16); // Set the pin low
		//printf( "hello\n" );
		uint8_t * buffer = (uint8_t*)usb_buffer_back;
		printf( " -- %08lx -- %08lx\n", plen, pcrc );
		int i;
		for( i = 0; i < 16; i++ )
			printf( "%02x ", buffer[i] );
		Delay_Ms( 100 );


		//Delay_Ms( 1000 );
		//dma_buffer[0] = 0;
		//dma_buffer[1] = 0;
		//printf( "%08lx %08lx\n", test_memory[0], test_memory[1] );
	}
}

