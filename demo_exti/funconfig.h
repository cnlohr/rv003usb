#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

#define FUNCONF_USE_DEBUGPRINTF 1
#define CH32V003                1
#define FUNCONF_SYSTICK_USE_HCLK 1
#define RV003_ADD_EXTI_MASK (1<<7)
#define RV003_ADD_EXTI_HANDLER \
				la a5, another_interrupt_flag; \
				li a0, 1; \
				sw a0, 0(a5); \

#endif

