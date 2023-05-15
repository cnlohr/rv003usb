// Could be defined here, or in the processor defines.
#define SYSTEM_CORE_CLOCK 48000000
#define SYSTICK_USE_HCLK

#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>

#define USB_DM 3
#define USB_DP 4
#define USBMASK 0x18181818
#define USB_PORT GPIOD
// Port D.3 = D+
// Port D.4 = D-
// Port D.5 = DPU

uint32_t wait_time = 0;

// DMA for shifting the GPIO into.
// Actually 2 sets of 4 uint8_t's
uint32_t dma_buffer[2];
uint32_t halfway_word;

void EXTI7_0_IRQHandler( void ) __attribute__((interrupt));
void EXTI7_0_IRQHandler( void ) 
{
	int i;
	for( i = 0; i < 100; i++ )
	{
		asm volatile( "\n\
			lw s0, 0(%[gpo])\n\
			li s0, 1\n\
			sw s0, 0(%[gpp])\n\
			li s0, 1<<16\n\
			sw s0, 0(%[gpp])\n\
		" : : [gpp]"r"(&GPIOC->BSHR), [gpo]"r"(&GPIOD->INDR) );
	}
}

int main()
{
	SystemInit48HSI();
	SetupDebugPrintf();
	SETUP_SYSTICK_HCLK

	// Enable GPIOs, DMA and TIMERs
	RCC->AHBPCENR = RCC_AHBPeriph_SRAM | RCC_AHBPeriph_DMA1;
	RCC->APB2PCENR = RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM1 | RCC_APB2Periph_GPIOA  | RCC_APB2Periph_AFIO;

	// GPIO C0 Push-Pull
	GPIOC->CFGLR &= ~(0xf<<(4*0));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*0);

	// GPIO D3 for input pin change.
	GPIOD->CFGLR =
		(GPIO_CNF_IN_PUPD)<<(4*1) |  // Keep SWIO enabled.
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF)<<(4*2) | // Output for Timer (T1C1)
		(GPIO_SPEED_IN | GPIO_CNF_IN_PUPD)<<(4*3) |  //PD3 = GPIOD IN
		(GPIO_SPEED_IN | GPIO_CNF_IN_PUPD)<<(4*4) |  //PD4 = GPIOD IN
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*5);

	// Configure the IO as an interrupt.
	AFIO->EXTICR = 3<<(4*2); //PORTD.3 (3 out front says PORTD, 3 in back says 3)
	EXTI->INTENR = 1<<4; // Enable EXT3
	EXTI->FTENR = 1<<4;  // Rising edge trigger

	// This drive GPIO5 high, which will tell the host that we are going on-bus.
	GPIOD->BSHR = 1<<5;

	// Disable fast interrupts.
	asm volatile( "addi t1,x0, 0\ncsrrw x0, 0x804, t1\n" : : :  "t1" );

	// enable interrupt
	NVIC_EnableIRQ( EXTI7_0_IRQn );

	while(1)
	{
		//GPIOC->BSHR = 1;   // Set pin high
		//Delay_Ms( 1000 );
		//GPIOC->BSHR = (1<<16); // Set the pin low
		//Delay_Ms( 1000 );
		int i;

		uint32_t amarka = halfway_word & 0x18181818;
		uint32_t amarkb = dma_buffer[1] & 0x18181818;
		printf( "test %08x %08x\n", amarka, amarkb );
		//dma_buffer[0] = 0;
		//dma_buffer[1] = 0;
	}
}
