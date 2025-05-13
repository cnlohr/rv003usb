// Implementation of platform specific functions in bitbang_rvswdio.h
//
// If you want the most up-to-date version of this, it will be stored in rv003usb.
//
// Copyright 2023-2024 <>< Charles Lohr, May be licensed under the MIT/x11 or
// NewBSD licenses. You choose. (Can be included in commercial and copyleft work)
// Assume 
//
// For RVBB_REMAP or default (great for QFN so you get CLKOUT optionally)
//        PC2+PC3 = SWIO (Plus 5.1k pull-up to PD2)
// Otherwise (Ideal for 8-pin SOIC, because PC4+PA1 are bound)
//        PC4+PA1 = SWIO (Plus 5.1k pull-up to PD2)
//

#if RVBB_REMAP
#define PIN_SWD_OUT PC2
#define PIN_SWD_IN  PC1
#define RVBB_TIM2   1
#define DMA_IN  DMA1_Channel7
#define DMA_OUT DMA1_Channel2
#else
#define PIN_SWD_OUT PA1
#define PIN_SWD_IN  PC4
#define RVBB_TIM1   1
#define DMA_IN  DMA1_Channel4
#define DMA_OUT DMA1_Channel5
#endif

//#define PIN_SWCLK   PC5     // Remappable.

#ifndef _BITBANG_RVSWDIO_CH32V003_H
#define _BITBANG_RVSWDIO_CH32V003_H

uint8_t command_buffer[64];  // to timer 1 ch 2
uint8_t reply_buffer[64];    // from timer 1 ch 4
static uint32_t buffptr = 0;

static void FinishBuffer(void);
static inline void Send1Bit(void);
static inline void Send0Bit(void);
static void IssueBuffer(void);

void ConfigureIOForRVSWD(void);
void ConfigureIOForRVSWIO(void);

// (when bit0 = 32 period, TPERIOD can be as low as 48, but that is risky)
// When bit0 = 40, TPERIOD CAN work as low as 50, but that's too risky.
// Let's bump it up to 56 => Period = 1.16us, but we're a little higher
#define TPERIOD 64

#define SWD_DELAY DelaySysTick(15)


static void FinishBuffer(void)
{
	while( DMA_OUT->CNTR ); // For T1C2. (TX)

	// Fix the first transaction here.
#if 0
	if( 0 )
	{
		printf( "%d %d %d %d - ", (int)DMA_OUT->CNTR, (int)DMA1_Channel4->CNTR, (int)TIM1->CH4CVR, (int)TIM1->CH3CVR );
		int i;
		for( i = 0; i < 64; i++ )
		{
			printf( "%d, ", (int)reply_buffer[i] );
		}
		printf( "\n" );
	}
#endif

}


static inline void Send1Bit(void)
{
	// 288-296ns low pulse. (Ideal period) - OK @ 2..15
	// Period is "technically" 166ns, but, it takes about 120ns to rise.
	command_buffer[buffptr++] = 10;
}

static inline void Send0Bit(void)
{
	// 888-904ns - OK @ 32 (At TPERIOD=48) - @48 (At TPERIOD=56)
	// Oddly, also works at =20 for TPERIOD=56
	// Period is "technically" 759ns, but, it takes about 120ns to rise.
	command_buffer[buffptr++] = 36;
}

static void IssueBuffer(void)
{
	// Note: command_buffer[0] must be 0, so that there is no race condition with reloading.
	// this buys us time in the beginning to find the up-going events.
	command_buffer[buffptr] = 0; // Turn PWM back to high.
	command_buffer[buffptr+1] = 0; // Make sure the USART RX is read to read.

	// Reset input.
	DMA_IN->CFGR &=~DMA_CFGR1_EN;
	DMA_IN->CNTR = buffptr+2;
	DMA_IN->CFGR |= DMA_CFGR1_EN;

	// Actually run output. 
	DMA_OUT->CNTR = buffptr+2;
}

////////////////////////////////////////////////////////////////
// For SWD
////////////////////////////////////////////////////////////////

static void SendBitRVSWD( int val )
{
	// Assume:
	// SWD is in indeterminte state.
	// SWC is HIGH
	funDigitalWrite( PIN_SWCLK, 0 );
	if( val )
	{
		funDigitalWrite( PIN_SWD_OUT, 1 );
	}
	else
	{
		funDigitalWrite( PIN_SWD_OUT, 0 );
	}
	funPinMode( PIN_SWD_OUT, GPIO_CFGLR_OUT_10Mhz_OD );
	SWD_DELAY;
	funDigitalWrite( PIN_SWCLK, 1 );
	SWD_DELAY;
}

static int ReadBitRVSWD( void )
{
	funPinMode( PIN_SWD_OUT, GPIO_CFGLR_IN_PUPD );
	funDigitalWrite( PIN_SWD_OUT, 1 );
	funDigitalWrite( PIN_SWCLK, 0 );
	SWD_DELAY;
	int r = !!(funDigitalRead( PIN_SWD_IN ));
	funDigitalWrite( PIN_SWCLK, 1 );
	SWD_DELAY;
	return r;
}

static void RVFinishRegop(void)
{
	ReadBitRVSWD( ); // ???
	ReadBitRVSWD( ); // ???
	ReadBitRVSWD( ); // ???
	SendBitRVSWD( 1 ); // 0 for register, 1 for value
	SendBitRVSWD( 0 ); // ??? Seems to have something to do with halting?


	funDigitalWrite( PIN_SWCLK, 0 );
	SWD_DELAY;
	funDigitalWrite( PIN_SWD_OUT, 0 );
	funPinMode( PIN_SWD_OUT, GPIO_CFGLR_OUT_50Mhz_PP );
	SWD_DELAY;
	funDigitalWrite( PIN_SWCLK, 1 );
	SWD_DELAY;
	funDigitalWrite( PIN_SWD_OUT, 1 );

	Delay_Us(2); // Sometimes 2 is too short.
}

static void MCFWriteReg32( struct SWIOState * state, uint8_t command, uint32_t value )
{
	if( state->opmode == 1 )
	{
		buffptr = 1;
		//printf( "WRITEREG: %02x %08x\n", command, value );
		Send1Bit();
		uint32_t mask;
		for( mask = 1<<6; mask; mask >>= 1 )
		{
			if( command & mask )
				Send1Bit();
			else
				Send0Bit();
		}
		Send1Bit( );
		for( mask = 1<<31; mask; mask >>= 1 )
		{
			if( value & mask )
				Send1Bit();
			else
				Send0Bit();
		}

		IssueBuffer();
		FinishBuffer();
	}
	else if( state->opmode == 2 )
	{
		uint32_t mask;
		funDigitalWrite( PIN_SWD_OUT, 0 );
		SWD_DELAY;
		int parity = 1;
		for( mask = 1<<6; mask; mask >>= 1 )
		{
			int v = !!(command & mask);
			parity ^= v;
			SendBitRVSWD( v );
		}
		SendBitRVSWD( 1 ); // Write = Set high
		SendBitRVSWD( parity );
		ReadBitRVSWD( ); // ???
		ReadBitRVSWD( ); // Seems only need to be set for first transaction (We are ignoring that though)
		ReadBitRVSWD( ); // ???
		SendBitRVSWD( 0 ); // 0 for register, 1 for value.
		SendBitRVSWD( 0 ); // ???  Seems to have something to do with halting.

		parity = 0;
		for( mask = 1<<31; mask; mask >>= 1 )
		{
			int v = !!(value & mask);
			parity ^= v;
			SendBitRVSWD( v );
		}
		SendBitRVSWD( parity );
		RVFinishRegop();
	}
}

// returns 0 if no error, otherwise error.
static int MCFReadReg32( struct SWIOState * state, uint8_t command, uint32_t * value )
{
	if( state->opmode == 1 )
	{
		buffptr = 1;
		Send1Bit();
		uint32_t mask;
		for( mask = 1<<6; mask; mask >>= 1 )
		{
			if( command & mask )
				Send1Bit();
			else
				Send0Bit();
		}
		Send0Bit( );
		int i;
		for( i = 0; i < 34; i++ )
		{
			Send1Bit();
		}

		IssueBuffer();
		FinishBuffer();

		// Need to wait one more bit time.
		Delay_Us(1);

		if( DMA_IN->CNTR != 3 )
		{
			printf( "RVR EXCEPT: %08x\n", (int)DMA_IN->CNTR );
			return -1;
		}

/*			for( i = 0; i < 48; i++ )
			{
				printf( "%d ", reply_buffer[i] );
			}
			printf( "\n" ); */
		uint32_t rv = 0;

		for( i = 9; i < 41; i++ )
		{
			// 11 to 48 for extremes.  Pick a middleground shifted smaller.
			// For slackier systems, the bottom end can move upward.
			// It is not unusual to see it go as high as 17 or 18.
			// This is the "length of the low pulse"
			rv = (rv << 1) | (reply_buffer[i] < 28);
			//printf( "%d ", (int)reply_buffer[i] );  Do this to check for sane values to compare against.
		}
		//printf( "RVR: %2x %08x\n", command, rv );
		memcpy( value, &rv, 4 );
	}
	else if( state->opmode == 2 )
	{
		int mask;
		funDigitalWrite( PIN_SWD_OUT, 0 );
		SWD_DELAY;
		int parity = 0;
		for( mask = 1<<6; mask; mask >>= 1 )
		{
			int v = !!(command & mask);
			parity ^= v;
			SendBitRVSWD( v );
		}
		SendBitRVSWD( 0 ); // Read = Set low
		SendBitRVSWD( parity );
		ReadBitRVSWD( ); // ???
		ReadBitRVSWD( ); // ???
		ReadBitRVSWD( ); // ???
		SendBitRVSWD( 0 ); // 0 for register, 1 for value
		SendBitRVSWD( 0 ); // ??? Seems to have something to do with halting?


		uint32_t rval = 0;
		int i;
		parity = 0;
		for( i = 0; i < 32; i++ )
		{
			rval <<= 1;
			int r = ReadBitRVSWD( );
			if( r == 1 )
			{
				rval |= 1;
				parity ^= 1;
			}
			if( r == 2 )
			{
				RVFinishRegop();
				return -1;
			}
		}
		memcpy( value, &rval, 4 );

		if( ReadBitRVSWD( ) != parity )
		{
			//BB_PRINTF_DEBUG( "Parity Failed\n" );
			RVFinishRegop();
			return -1;
		}

		RVFinishRegop();
	}
	return 0;
}

void ConfigureIOForRVSWD(void)
{
	funDigitalWrite( PIN_SWCLK, 1 );
	funPinMode( PIN_SWCLK, GPIO_CFGLR_OUT_10Mhz_PP );
	funDigitalWrite( PIN_SWD_OUT, 1 );
	funPinMode( PIN_SWD_OUT, GPIO_CFGLR_OUT_10Mhz_PP );
	funDigitalWrite( PIN_SWD_IN, 1 );
	funPinMode( PIN_SWD_IN, GPIO_CFGLR_IN_PUPD );
}

void ConfigureIOForRVSWIO(void)
{
	// Enable GPIOC, GPIOD and TIM1, as wel as DMA
	#if RVBB_TIM2
	RCC->APB1PCENR |= RCC_APB1Periph_TIM2;
	#elif RVBB_TIM1
	RCC->APB2PCENR |= RCC_APB2Periph_TIM1;
	#else
	#error Impossible configuration.
	#endif
	RCC->APB2PCENR |= RCC_APB2Periph_AFIO;
	RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;

	funDigitalWrite( PIN_SWD_OUT, 1 );
	funDigitalWrite( PIN_SWD_IN, 1 );

	// Now, PA1 and PC4 are hard connected.

	funPinMode( PIN_SWD_OUT, GPIO_CFGLR_OUT_10Mhz_AF_OD ); // Timer Channel 4
	funPinMode( PIN_SWD_IN, GPIO_CFGLR_IN_PUPD );          // Timer Channel 2

#if RVBB_TIM2
	AFIO->PCFR1 |= AFIO_PCFR1_TIM2_REMAP_PARTIALREMAP1;

	// SMCFGR: default clk input is CK_INT
	// set TIM1 clock prescaler divider 
	TIM2->PSC = 0x0000;
	// set PWM total cycle width
	TIM2->ATRLR = TPERIOD;

	TIM2->CH2CVR = 0; // Release output (Allow it to be high)

	// CH2 Mode is output, PWM1 (CC1S = 00, OC1M = 110)
	TIM2->CHCTLR1 = TIM_OC2M_2 | TIM_OC2M_1 | TIM_OC2PE;
	TIM2->CHCTLR2 = TIM_CC4S_0;  // CH4 in, can't add glitch filter otherwise it delays the next event.

	// Enable CH4 output, positive pol
	TIM2->CCER = TIM_CC2E | TIM_CC2P | TIM_CC4E | TIM_CC3E;

	// No MOE on TIM2.
	//TIM2->BDTR = TIM_MOE;

	// initialize counter
	TIM2->SWEVGR = TIM_UG;

	// Enable TIM2
	TIM2->CTLR1 = TIM_ARPE | TIM_CEN;
	TIM2->DMAINTENR = TIM_CC4DE | TIM_UDE; // Enable DMA motion.
	TIM2->CTLR1 |= TIM_URS;   // Trigger DMA on overflow ONLY.
	DMA_IN->PADDR = (uint32_t)&TIM2->CH4CVR; // Input
	DMA_OUT->PADDR = (uint32_t)&TIM2->CH2CVR; // Output

#elif RVBB_TIM1
	// SMCFGR: default clk input is CK_INT
	// set TIM1 clock prescaler divider 
	TIM1->PSC = 0x0000;
	// set PWM total cycle width
	TIM1->ATRLR = TPERIOD;

	TIM1->CH2CVR = 0; // Release output (Allow it to be high)

	// CH2 Mode is output, PWM1 (CC1S = 00, OC1M = 110)
	TIM1->CHCTLR1 = TIM_OC2M_2 | TIM_OC2M_1 | TIM_OC2PE;
	TIM1->CHCTLR2 = TIM_CC4S_0;  // CH4 in, can't add glitch filter otherwise it delays the next event.

	// Enable CH4 output, positive pol
	TIM1->CCER = TIM_CC2E | TIM_CC2P | TIM_CC4E | TIM_CC3E;

	// Enable TIM1 outputs
	TIM1->BDTR = TIM_MOE;
	
	// initialize counter
	TIM1->SWEVGR = TIM_UG;

	// Enable TIM2
	TIM1->CTLR1 = TIM_ARPE | TIM_CEN;
	TIM1->DMAINTENR = TIM_CC4DE | TIM_UDE; // Enable DMA motion.
	TIM1->CTLR1 |= TIM_URS;   // Trigger DMA on overflow ONLY.
	DMA_IN->PADDR = (uint32_t)&TIM1->CH4CVR; // Input
	DMA_OUT->PADDR = (uint32_t)&TIM1->CH2CVR; // Output
#endif

	//TIM1_UP
	DMA_OUT->MADDR = (uint32_t)command_buffer;
	DMA_OUT->CFGR = 
		DMA_CFGR1_DIR     |                  // MEM2PERIPHERAL
		0                 |                  // Low priority.
		0                 |                  // 8-bit memory
		DMA_CFGR1_PSIZE_0                 |  // 16-bit peripheral
		DMA_CFGR1_MINC    |                  // Increase memory.
		0                 |                  // NOT Circular mode.
		0                 |                  // NO Half-trigger
		0                 |                  // NO Whole-trigger
		DMA_CFGR1_EN;                        // Enable

	// TIM1_TRIG/TIM1_COM/TIM1_CH4
	DMA_IN->MADDR = (uint32_t)reply_buffer;
	DMA_IN->CFGR = 
		0                 |                  // PERIPHERAL to MEMORY
		0                 |                  // Low priority.
		0                 |                  // 8-bit memory
		DMA_CFGR1_PSIZE_0                 |                  // 16-bit peripheral
		DMA_CFGR1_MINC    |                  // Increase memory.
		0                 |                  // NOT Circular mode.
		0                 |                  // NO Half-trigger
		0                 |                  // NO Whole-trigger
		DMA_CFGR1_EN;                        // Enable

}

#endif

