#include <ch32fun.h>
#include <stdio.h>
#include "../rv003usb/rv003usb.h"

uint32_t count;

volatile int last = 0;
void handle_debug_input(int numbytes, uint8_t * data) {
	last = data[0];
	count += numbytes;
}

int main() {
  SystemInit();

  RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO;
  
  SysTick->CNT = 0;
  Delay_Ms(1); // Ensures USB re-enumeration after bootloader or reset; Spec demand >2.5µs ( TDDIS )
  
  usb_setup();

  count = 0;

  // GPIO D0 Push-Pull
	GPIOD->CFGLR &= ~(0xf<<(4*0));
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*0);

	// GPIO C0 Push-Pull
	GPIOC->CFGLR &= ~(0xf<<(4*0));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*0);

  while (!DidDebuggerAttach());
  
  while(1) {
    GPIOD->BSHR = 1;	 // Turn on GPIOs
		GPIOC->BSHR = 1;
    printf("+%lu\n", count++);
    Delay_Ms(100);
    GPIOD->BSHR = (1<<16); // Turn off GPIODs
		GPIOC->BSHR = (1<<16);
    printf("-%lu[%c]\n", count++, last);
    Delay_Ms(100);
  }
}