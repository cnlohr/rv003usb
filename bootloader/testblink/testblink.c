#include "ch32v003fun.h"

//XXX TODO Make this so it can "run from main"
void Scratchpad( uint8_t * buffer, uint32_t * keeprunning )
{
	GPIOC->CFGLR = (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*0);
	int i;
for( i = 0; i < 8; i++ )
		GPIOC->BSHR = 1<<0;
	GPIOC->BSHR = 1<<16;
}

