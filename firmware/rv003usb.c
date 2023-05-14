// Could be defined here, or in the processor defines.
#define SYSTEM_CORE_CLOCK 48000000
#define SYSTICK_USE_HCLK

#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>

#define USB_DM 3
#define USB_DP 4
#define USB_PORT GPIOD
// Port D.3 = D+
// Port D.4 = D-
// Port D.5 = DPU

uint32_t wait_time = 0;

// DMA for shifting the GPIO into.
uint8_t dma_buffer[128];

void EXTI7_0_IRQHandler( void ) __attribute__((interrupt));
void EXTI7_0_IRQHandler( void ) 
{
/*	if( !(GPIOD->INDR & ((1<<USB_DM)|(1<<USB_DP))) )
	{
		// EOP. (TODO!)	
		EXTI->INTFR = 1<<4;
		return;
	}
*/
	TIM1->CTLR1 = TIM_CEN;
	GPIOC->BSHR = 1;
/*
	while( dma_buffer[0] == 0xff )
	{
		;
	}
*/
	Delay_Us(10);
	TIM1->CTLR1 = 0;
	GPIOC->BSHR = 1<<16;
	
	DMA1_Channel2->CNTR = sizeof(dma_buffer) / sizeof(dma_buffer[0]);
	DMA1_Channel2->MADDR = (uint32_t)dma_buffer;
	DMA1_Channel2->CFGR |= DMA_CFGR1_EN;
	dma_buffer[0] = 0xff;
//	Delay_Us(10);
	EXTI->INTFR = 1<<4;
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


	// DMA2 can be configured to attach to T1CH1
	// The system can only DMA out at ~2.2MSPS.  2MHz is stable.
	DMA1_Channel2->CNTR = sizeof(dma_buffer) / sizeof(dma_buffer[0]);
	DMA1_Channel2->MADDR = (uint32_t)dma_buffer;
	DMA1_Channel2->PADDR = (uint32_t)&GPIOD->INDR;
	DMA1_Channel2->CFGR = 
		0 |                      // MEM2PERIPHERAL (DMA_CFGR1_DIR) = 0
		DMA_CFGR1_PL |                       // High priority.
		0 |                                  // 8-bit memory
		0 |                                  // 8-bit peripheral
		DMA_CFGR1_MINC |                     // Increase memory.
		0 |                                  // Circular mode = 0 not circular
		0 |                     // Half-trigger (no )
		0 |                     // Whole-trigger (no)
		DMA_CFGR1_EN;                        // Enable

//	NVIC_EnableIRQ( DMA1_Channel2_IRQn );
	DMA1_Channel2->CFGR |= DMA_CFGR1_EN;

	// NOTE: You can also hook up DMA1 Channel 3 to T1C2,
	// if you want to output to multiple IO ports at
	// at the same time.  Just be aware you have to offset
	// the time they read at by at least 1/8Mth of a second.

	// Setup Timer1.
	RCC->APB2PRSTR = RCC_APB2Periph_TIM1;    // Reset Timer
	RCC->APB2PRSTR = 0;

	// Timer 1 setup.
	TIM1->PSC = 0x0000;                      // Prescaler 
	TIM1->ATRLR = 31;                        // Auto Reload - sets period (48MHz / (11+1) = 4MHz)
	TIM1->SWEVGR = TIM_UG | TIM_TG;          // Reload immediately + Trigger DMA
	TIM1->CCER = TIM_CC1E | TIM_CC1P;        // Enable CH1 output, positive pol
	TIM1->CHCTLR1 = TIM_OC1M_2 | TIM_OC1M_1; // CH1 Mode is output, PWM1 (CC1S = 00, OC1M = 110)
	TIM1->CH1CVR = 1;                        // Set the Capture Compare Register value to 50% initially
	TIM1->BDTR = TIM_MOE;                    // Enable TIM1 outputs
	TIM1->CTLR1 = 0;                   // Enable TIM1
	TIM1->DMAINTENR = TIM_UDE | TIM_CC1DE;   // Trigger DMA on TC match 1 (DMA Ch2) and TC match 2 (DMA Ch3)

	memset( dma_buffer, 0xff, sizeof( dma_buffer ) );

	// enable interrupt
	NVIC_EnableIRQ( EXTI7_0_IRQn );

	while(1)
	{
		//GPIOC->BSHR = 1;   // Set pin high
		Delay_Ms( 1000 );
		//GPIOC->BSHR = (1<<16); // Set the pin low
		Delay_Ms( 1000 );
		int i;

		//NVIC_DisableIRQ( EXTI7_0_IRQn );

		printf( "test %08x\n", dma_buffer[0] );

		//NVIC_EnableIRQ( EXTI7_0_IRQn );
	}
}
