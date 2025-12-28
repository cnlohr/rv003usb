#include "ch32fun.h"

//XXX TODO Make this so it can "run from main"
void Scratchpad( uint8_t * buffer, uint32_t * keeprunning )
{
	GPIOC->CFGLR = (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*0);

	if( SysTick->CNT & 0x1000000 )
	{
		GPIOC->BSHR = 1<<0;
	}
	else
	{
		GPIOC->BSHR = 1<<16;
	}
}

