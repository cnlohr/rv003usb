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

static void DoneTransaction()
{
	TIM1->CTLR1 = 0;
	TIM1->CNT = 0;
	uint32_t temp = DMA1_Channel2->CFGR;
	DMA1_Channel2->CFGR = temp & ~DMA_CFGR1_EN;
	DMA1_Channel2->CNTR = sizeof(dma_buffer);
	DMA1_Channel2->CFGR = temp;
	NVIC_EnableIRQ( EXTI7_0_IRQn );
}

static void HandleWord( uint32_t word )
{
	static uint32_t state;
	uint32_t localstate = state;

	GPIOC->BSHR = 1<<16;
	GPIOC->BSHR = 1;
	word &= USBMASK;
	int i;
	for( i = 0; i < 4; i++ )
	{
		if( ( word & 0xff ) == 0 )
		{
			DoneTransaction();
			// SE0?
		}
		word >>= 8;
	}
	halfway_word = word;

	state = localstate;
}


void DMA1_Channel2_IRQHandler( void ) __attribute__((interrupt));
void DMA1_Channel2_IRQHandler( void ) 
{
	GPIOC->BSHR = 1;
	int i;
	static int frameno;
	volatile int intfr = DMA1->INTFR;
	do
	{
		DMA1->INTFCR = DMA1_IT_GL2;

		// Gets called at the end-of-a frame.
		if( intfr & DMA1_IT_TC2 )
		{
			HandleWord( dma_buffer[1] );
		}
		
		// Gets called halfway through the frame
		if( intfr & DMA1_IT_HT2 )
		{
			HandleWord( dma_buffer[0] );
		}
		intfr = DMA1->INTFR;
	} while( intfr );
	GPIOC->BSHR = 1<<16;
}


void EXTI7_0_IRQHandler( void ) __attribute__((interrupt));
void EXTI7_0_IRQHandler( void ) 
{
	TIM1->CTLR1 = TIM_CEN;
	NVIC_DisableIRQ( EXTI7_0_IRQn );
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

	// Disable fast interrupts.
	asm volatile( "addi t1,x0, 0\ncsrrw x0, 0x804, t1\n" : : :  "t1" );

	// DMA2 can be configured to attach to T1CH1
	// The system can only DMA out at ~2.2MSPS.  2MHz is stable.
	DMA1_Channel2->CNTR = sizeof(dma_buffer);
	DMA1_Channel2->MADDR = (uint32_t)dma_buffer;
	DMA1_Channel2->PADDR = (uint32_t)&GPIOD->INDR;
	DMA1_Channel2->CFGR = 
		0 |                      // MEM2PERIPHERAL (DMA_CFGR1_DIR) = 0
		DMA_CFGR1_PL |                       // High priority.
		0 |                                  // 8-bit memory
		0 |                                  // 8-bit peripheral
		DMA_CFGR1_MINC |                     // Increase memory.
		DMA_CFGR1_CIRC |                                  // Circular mode = 1 not circular
		DMA_CFGR1_HTIE |                     // Half-trigger (yes)
		DMA_CFGR1_TCIE |                     // Whole-trigger (yes)
		DMA_CFGR1_EN;                        // Enable

	NVIC_EnableIRQ( DMA1_Channel2_IRQn );
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
	TIM1->CTLR1 = 0;                         // Enable TIM1
	TIM1->DMAINTENR = TIM_UDE | TIM_CC1DE;   // Trigger DMA on TC match 1 (DMA Ch2) and TC match 2 (DMA Ch3)

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
