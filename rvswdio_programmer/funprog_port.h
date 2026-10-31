#ifndef FUNPROG_PORT_H
#define FUNPROG_PORT_H

// Define your debug print method here
#if DEBUG_PRINT
#define FUN_PRINTF_DEBUG(format, args...) printf(format, ##args)
#else
#define FUN_PRINTF_DEBUG(format, args...)
#endif

#if STATUS_LED
#define STATUS_LED_ON funDigitalWrite( STATUS_LED, STATUS_LED_ON_POLARITY );
#define STATUS_LED_OFF funDigitalWrite( STATUS_LED, STATUS_LED_OFF_POLARITY );
#else
#define STATUS_LED_ON
#define STATUS_LED_OFF
#endif

#define POWER_SETUP do {funPinMode( PIN_TARGETPOWER, PIN_TARGETPOWER_MODE ); \
                        funDigitalWrite( PIN_TARGETPOWER, POWER_ON_POLARITY );} while(0)
#define POWER_ON POWER_SETUP
#define POWER_OFF funDigitalWrite( PIN_TARGETPOWER, POWER_OFF_POLARITY );

#define DELAY_US(n) Delay_Us(n)
#define DELAY_MS(n) Delay_Ms(n)

// Some chips need to slow down the protocol speed to work stable. Check available implementations
#define SLOW_DOWN_PROTOCOL

#define IRAM

// Implement the function to enable clock output on a dedicated pin
#if defined(MCO_SUPPORT) && MCO_SUPPORT
static void EnableMCO( int use_apll, int sdm0, int sdm1, int sdm2, int odiv )
{
	// Output MCO clock on PC4.
	if( use_apll )
	{
		RCC->CFGR0 = (RCC->CFGR0 & (~(7<<24))) | (sdm0<<24) | (1<<26);
		// 0 = SYSCLK, 1 = RC, 2 = HSE, 3 = PLL
		funPinMode( PC4, GPIO_CFGLR_OUT_50Mhz_AF_PP );
	}
	else
	{
		RCC->CFGR0 &= ~(7<<24);
	}
}
#endif

#endif
