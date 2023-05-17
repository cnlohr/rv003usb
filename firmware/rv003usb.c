// Could be defined here, or in the processor defines.
#define SYSTEM_CORE_CLOCK 48000000
#define SYSTICK_USE_HCLK

#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

uint32_t test_memory[2];

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
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*DEBUG_PIN) |
		(GPIO_SPEED_IN | GPIO_CNF_IN_PUPD)<<(4*USB_DM) |  //PD3 = GPIOD IN
		(GPIO_SPEED_IN | GPIO_CNF_IN_PUPD)<<(4*USB_DP) |  //PD4 = GPIOD IN
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*USB_DPU);

	// Configure the IO as an interrupt.
	AFIO->EXTICR = 3<<(USB_DP*2); //PORTD.3 (3 out front says PORTD, 3 in back says 3)
	EXTI->INTENR = 1<<USB_DP; // Enable EXT3
	EXTI->FTENR = 1<<USB_DP;  // Rising edge trigger

	// This drive GPIO5 high, which will tell the host that we are going on-bus.
	GPIOD->BSHR = 1<<USB_DPU;

<<<<<<< HEAD
	// Disable fast interrupts. "HPE"
	asm volatile( "addi t1,x0, 3\ncsrrw x0, 0x804, t1\n" : : :  "t1" );

=======
>>>>>>> 7db91ac5f5e4e82a0a7a8e20f71773b518919496
	// enable interrupt
	NVIC_EnableIRQ( EXTI7_0_IRQn );

	while(1)
	{
		GPIOC->BSHR = 1;   // Set pin high
		Delay_Ms( 1000 );
		GPIOC->BSHR = (1<<16); // Set the pin low
		//Delay_Ms( 1000 );
		//dma_buffer[0] = 0;
		//dma_buffer[1] = 0;
		//printf( "%08lx %08lx\n", test_memory[0], test_memory[1] );
	}
}
