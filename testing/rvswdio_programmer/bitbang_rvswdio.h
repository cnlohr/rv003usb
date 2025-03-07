// CH32V003 SWIO / CH32Vxxx / CH32Xxxx bit banging minimal reference
// implementation for bit banged IO or primitive IO.
//
// You will need to implement some of the basic functions. Example
// implementations for the functions on the ESP32-S2 are included.
//
// Copyright 2023-2024 <>< Charles Lohr, May be licensed under the MIT/x11 or
// NewBSD licenses. You choose. (Can be included in commercial and copyleft work)
//
// This file is original work.
//
// Mostly tested, though, not may not be perfect. Expect to tweak some things.
// The 003 code is pretty hardened, the other code may not be.
//
// The only resources used to produce this file were publicly available docs
// and playing with the chips.  The only leveraged tool was this:
//   https://github.com/perigoso/sigrok-rvswd

#ifndef _WCH_RVSWD_RVSWIO_H
#define _WCH_RVSWD_RVSWIO_H

// This is a hacky thing, but if you are laaaaazzzyyyy and don't want to add a 10k
// resistor, youcan do this.  It glitches the line high very, very briefly.  
// Enable for when you don't have a 10k pull-upand are relying on the internal pull-up.
// WARNING: If you set this, you should set the drive current to 5mA.  This is 
// only useful / needed if you are using the built-in functions.
//#define R_GLITCH_HIGH
//#define RUNNING_ON_ESP32S2
//#define BB_PRINTF_DEBUG uprintf

// You should interface to this file via these functions

enum RiscVChip {
	CHIP_UNKNOWN = 0x00,
	CHIP_CH32V10x = 0x01,
	CHIP_CH57x = 0x02,
	CHIP_CH56x = 0x03,
	CHIP_CH32V20x = 0x05,
	CHIP_CH32V30x = 0x06,
	CHIP_CH58x = 0x07,
	CHIP_CH32V003 = 0x09,
	CHIP_CH32X03x = 0x0d,


	CHIP_CH32V002 = 0x22,
	CHIP_CH32V004 = 0x24,
	CHIP_CH32V005 = 0x25,
	CHIP_CH32V006 = 0x26,
};

struct SWIOState
{
	// Set these before calling any functions
	int t1coeff;
	int pinmaskD;
	int pinmaskC;
	int opmode; // 0 for undefined, 1 for SWIO, 2 for SWD
	enum RiscVChip target_chip_type;
	int sectorsize;

	// Zero the rest of the structure.
	uint32_t statetag;
	uint32_t lastwriteflags;
	uint32_t currentstateval;
	uint32_t flash_unlocked;
	uint32_t autoincrement;
};

#define STTAG( x ) (*((uint32_t*)(x)))

#define IRAM IRAM_ATTR

#ifdef RUNNING_ON_ESP32S2
// You may need to rewrite, depending on your architecture.
static inline void Send1BitSWIO( int t1coeff, int pinmaskD ) IRAM;
static inline void Send0BitSWIO( int t1coeff, int pinmaskD ) IRAM;
static inline int ReadBitSWIO( struct SWIOState * state ) IRAM;
static inline void SendBitRVSWD( int t1coeff, int pinmaskD, int pinmaskC, int val ) IRAM;
static inline int ReadBitRVSWD( int t1coeff, int pinmaskD, int pinmaskC ) IRAM;
#else
static inline void ConfigureIOForRVSWIO(void);
static inline void ConfigureIOForRVSWD(void);
#endif

// Provided Basic functions
static void MCFWriteReg32( struct SWIOState * state, uint8_t command, uint32_t value ) IRAM;
static int MCFReadReg32( struct SWIOState * state, uint8_t command, uint32_t * value ) IRAM;

// More advanced functions built on lower level PHY.
static int InitializeSWDSWIO( struct SWIOState * state );
static int ReadWord( struct SWIOState * state, uint32_t word, uint32_t * ret );
static int WriteWord( struct SWIOState * state, uint32_t word, uint32_t val );
static int WaitForFlash( struct SWIOState * state );
static int WaitForDoneOp( struct SWIOState * state );
static int Write64Block( struct SWIOState * iss, uint32_t address_to_write, uint8_t * data );
static int UnlockFlash( struct SWIOState * iss );
static int EraseFlash( struct SWIOState * iss, uint32_t address, uint32_t length, int type );
static void ResetInternalProgrammingState( struct SWIOState * iss );
static int PollTerminal( struct SWIOState * iss, uint8_t * buffer, int maxlen, uint32_t leavevalA, uint32_t leavevalB );

#define BDMDATA0        0x04
#define BDMDATA1        0x05
#define DMCONTROL      0x10
#define DMSTATUS       0x11
#define DMHARTINFO     0x12
#define DMABSTRACTCS   0x16
#define DMCOMMAND      0x17
#define DMABSTRACTAUTO 0x18
#define DMPROGBUF0     0x20
#define DMPROGBUF1     0x21
#define DMPROGBUF2     0x22
#define DMPROGBUF3     0x23
#define DMPROGBUF4     0x24
#define DMPROGBUF5     0x25
#define DMPROGBUF6     0x26
#define DMPROGBUF7     0x27

#define DMCPBR       0x7C
#define DMCFGR       0x7D
#define DMSHDWCFGR   0x7E

#ifndef __CH32V00x_H
#define FLASH_STATR_WRPRTERR       ((uint8_t)0x10) 
#define CR_PAGE_PG                 ((uint32_t)0x00010000)
#define CR_BUF_LOAD                ((uint32_t)0x00040000)
#ifndef FLASH_CTLR_MER
#define FLASH_CTLR_MER             ((uint16_t)0x0004)     /* Mass Erase */
#endif
#define CR_STRT_Set                ((uint32_t)0x00000040)
#define CR_PAGE_ER                 ((uint32_t)0x00020000)
#define CR_BUF_RST                 ((uint32_t)0x00080000)
#endif

#ifdef RUNNING_ON_ESP32S2

static inline void PrecDelay( int delay )
{
	asm volatile( 
"1:	addi %[delay], %[delay], -1\n"
"	bbci %[delay], 31, 1b\n" : [delay]"+r"(delay)  );
}

// TODO: Add continuation (bypass) functions.
// TODO: Consider adding parity bit (though it seems rather useless)

// All three bit functions assume bus state will be in
// 	GPIO.out_w1ts = pinmask;
//	GPIO.enable_w1ts = pinmask;
// when they are called.
static inline void Send1BitSWIO( int t1coeff, int pinmaskD )
{
	// Low for a nominal period of time.
	// High for a nominal period of time.

	GPIO.out_w1tc = pinmaskD;
	PrecDelay( t1coeff );
	GPIO.out_w1ts = pinmaskD;
	PrecDelay( t1coeff );
}

static inline void Send0BitSWIO( int t1coeff, int pinmaskD )
{
	// Low for a LONG period of time.
	// High for a nominal period of time.
	int longwait = t1coeff*4;
	GPIO.out_w1tc = pinmaskD;
	PrecDelay( longwait );
	GPIO.out_w1ts = pinmaskD;
	PrecDelay( t1coeff );
}

// returns 0 if 0
// returns 1 if 1
// returns 2 if timeout.
static inline int ReadBitSWIO( struct SWIOState * state )
{
	int t1coeff = state->t1coeff;
	int pinmaskD = state->pinmaskD;

	// Drive low, very briefly.  Let drift high.
	// See if CH32V003 is holding low.

	int timeout = 0;
	int ret = 0;
	int medwait = t1coeff * 2;
	GPIO.out_w1tc = pinmaskD;
	PrecDelay( t1coeff );
	GPIO.enable_w1tc = pinmaskD;
	GPIO.out_w1ts = pinmaskD;
#ifdef R_GLITCH_HIGH
	int halfwait = t1coeff / 2;
	PrecDelay( halfwait );
	GPIO.enable_w1ts = pinmaskD;
	GPIO.enable_w1tc = pinmaskD;
	PrecDelay( halfwait );
#else
	PrecDelay( medwait );
#endif
	ret = GPIO.in;

#ifdef R_GLITCH_HIGH
	if( !(ret & pinmaskD) )
	{
		// Wait if still low.
		PrecDelay( medwait );
		GPIO.enable_w1ts = pinmaskD;
		GPIO.enable_w1tc = pinmaskD;
	}
#endif
	for( timeout = 0; timeout < MAX_IN_TIMEOUT; timeout++ )
	{
		if( GPIO.in & pinmaskD )
		{
			GPIO.enable_w1ts = pinmaskD;
			int fastwait = t1coeff / 2;
			PrecDelay( fastwait );
			return !!(ret & pinmaskD);
		}
	}
	
	// Force high anyway so, though hazarded, we can still move along.
	GPIO.enable_w1ts = pinmaskD;
	return 2;
}


static inline void SendBitRVSWD( int t1coeff, int pinmaskD, int pinmaskC, int val )
{
	// Assume:
	// SWD is in indeterminte state.
	// SWC is HIGH
	GPIO.out_w1tc = pinmaskC;
	if( val )
		GPIO.out_w1ts = pinmaskD;
	else
		GPIO.out_w1tc = pinmaskD;
	GPIO.enable_w1ts = pinmaskD;
	PrecDelay( t1coeff );
	GPIO.out_w1ts = pinmaskC;
	PrecDelay( t1coeff );
}

static inline int ReadBitRVSWD( int t1coeff, int pinmaskD, int pinmaskC )
{
	GPIO.enable_w1tc = pinmaskD;
	GPIO.out_w1tc = pinmaskC;
	GPIO.out_w1ts = pinmaskD;
	PrecDelay( t1coeff );
	int r = !!(GPIO.in & pinmaskD);
	GPIO.out_w1ts = pinmaskC;
	PrecDelay( t1coeff );
	return r;
}


static void MCFWriteReg32( struct SWIOState * state, uint8_t command, uint32_t value )
{
	int t1coeff = state->t1coeff;
	int pinmaskD = state->pinmaskD;
	int pinmaskC = state->pinmaskC;
 	GPIO.out_w1ts = pinmaskC;
	GPIO.enable_w1ts = pinmaskC;
 	GPIO.out_w1ts = pinmaskD;
	GPIO.enable_w1ts = pinmaskD;
	//BB_PRINTF_DEBUG( "CO: (%08x=>%08x) %08x %08x %d %d\n", command, value, pinmaskD, pinmaskC, t1coeff, state->opmode );
	if( state->opmode == 1 )
	{
		DisableISR();
		Send1BitSWIO( t1coeff, pinmaskD );
		uint32_t mask;
		for( mask = 1<<6; mask; mask >>= 1 )
		{
			if( command & mask )
				Send1BitSWIO( t1coeff, pinmaskD );
			else
				Send0BitSWIO( t1coeff, pinmaskD );
		}
		Send1BitSWIO( t1coeff, pinmaskD );
		for( mask = 1<<31; mask; mask >>= 1 )
		{
			if( value & mask )
				Send1BitSWIO( t1coeff, pinmaskD );
			else
				Send0BitSWIO( t1coeff, pinmaskD );
		}
		EnableISR();
		esp_rom_delay_us(8); // Sometimes 2 is too short.
	}
	else if( state->opmode == 2 )
	{
		uint32_t mask;
		DisableISR();
	 	GPIO.out_w1tc = pinmaskD;
		PrecDelay( t1coeff );
		int parity = 1;
		for( mask = 1<<6; mask; mask >>= 1 )
		{
			int v = !!(command & mask);
			parity ^= v;
			SendBitRVSWD( t1coeff, pinmaskD, pinmaskC, v );
		}
		SendBitRVSWD( t1coeff, pinmaskD, pinmaskC, 1 ); // Write = Set high
		SendBitRVSWD( t1coeff, pinmaskD, pinmaskC, parity );
		ReadBitRVSWD( t1coeff, pinmaskD, pinmaskC ); // ???
		ReadBitRVSWD( t1coeff, pinmaskD, pinmaskC ); // Seems only need to be set for first transaction (We are ignoring that though)
		ReadBitRVSWD( t1coeff, pinmaskD, pinmaskC ); // ???
		SendBitRVSWD( t1coeff, pinmaskD, pinmaskC, 0 ); // 0 for register, 1 for value.
		SendBitRVSWD( t1coeff, pinmaskD, pinmaskC, 0 ); // ???  Seems to have something to do with halting.


		parity = 0;
		for( mask = 1<<31; mask; mask >>= 1 )
		{
			int v = !!(value & mask);
			parity ^= v;
			SendBitRVSWD( t1coeff, pinmaskD, pinmaskC, v );
		}
		SendBitRVSWD( t1coeff, pinmaskD, pinmaskC, parity );
		ReadBitRVSWD( t1coeff, pinmaskD, pinmaskC ); // ???
		ReadBitRVSWD( t1coeff, pinmaskD, pinmaskC ); // ???
		ReadBitRVSWD( t1coeff, pinmaskD, pinmaskC ); // ???
		SendBitRVSWD( t1coeff, pinmaskD, pinmaskC, 1 ); // 0 for register, 1 for value
		SendBitRVSWD( t1coeff, pinmaskD, pinmaskC, 0 ); // ??? Seems to have something to do with halting?


		GPIO.out_w1tc = pinmaskC;
		PrecDelay( t1coeff );
		GPIO.out_w1tc = pinmaskD;
		GPIO.enable_w1ts = pinmaskD;
		PrecDelay( t1coeff );
		GPIO.out_w1ts = pinmaskC;
		PrecDelay( t1coeff );
		GPIO.out_w1ts = pinmaskD;

		EnableISR();
		esp_rom_delay_us(8); // Sometimes 2 is too short.
	}
}

// returns 0 if no error, otherwise error.
static int MCFReadReg32( struct SWIOState * state, uint8_t command, uint32_t * value )
{
	int t1coeff = state->t1coeff;
	int pinmaskD = state->pinmaskD;
	int pinmaskC = state->pinmaskC;
 	GPIO.out_w1ts = pinmaskC;
	GPIO.enable_w1ts = pinmaskC;
 	GPIO.out_w1ts = pinmaskD;
	GPIO.enable_w1ts = pinmaskD;

	if( state->opmode == 1 )
	{
		DisableISR();
		Send1BitSWIO( t1coeff, pinmaskD );
		int i;
		uint32_t mask;
		for( mask = 1<<6; mask; mask >>= 1 )
		{
			if( command & mask )
				Send1BitSWIO(t1coeff, pinmaskD);
			else
				Send0BitSWIO(t1coeff, pinmaskD);
		}
		Send0BitSWIO( t1coeff, pinmaskD );
		uint32_t rval = 0;
		for( i = 0; i < 32; i++ )
		{
			rval <<= 1;
			int r = ReadBitSWIO( state );
			if( r == 1 )
				rval |= 1;
			if( r == 2 )
			{
				EnableISR();
				return -21;
			}
		}
		*value = rval;
		EnableISR();
		esp_rom_delay_us(8); // Sometimes 2 is too short.
	}
	else if( state->opmode == 2 )
	{
		int mask;
		DisableISR();
	 	GPIO.out_w1tc = pinmaskD;
		PrecDelay( t1coeff );
		int parity = 0;
		for( mask = 1<<6; mask; mask >>= 1 )
		{
			int v = !!(command & mask);
			parity ^= v;
			SendBitRVSWD( t1coeff, pinmaskD, pinmaskC, v );
		}
		SendBitRVSWD( t1coeff, pinmaskD, pinmaskC, 0 ); // Read = Set low
		SendBitRVSWD( t1coeff, pinmaskD, pinmaskC, parity );
		ReadBitRVSWD( t1coeff, pinmaskD, pinmaskC ); // ???
		ReadBitRVSWD( t1coeff, pinmaskD, pinmaskC ); // ???
		ReadBitRVSWD( t1coeff, pinmaskD, pinmaskC ); // ???
		SendBitRVSWD( t1coeff, pinmaskD, pinmaskC, 0 ); // 0 for register, 1 for value
		SendBitRVSWD( t1coeff, pinmaskD, pinmaskC, 0 ); // ??? Seems to have something to do with halting?


		uint32_t rval = 0;
		int i;
		parity = 0;
		for( i = 0; i < 32; i++ )
		{
			rval <<= 1;
			int r = ReadBitRVSWD( t1coeff, pinmaskD, pinmaskC );
			if( r == 1 )
			{
				rval |= 1;
				parity ^= 1;
			}
			if( r == 2 )
			{
				EnableISR();
				return -21;
			}
		}
		*value = rval;

		if( ReadBitRVSWD( t1coeff, pinmaskD, pinmaskC ) != parity )
		{
			EnableISR();
			return -23;
		}

		ReadBitRVSWD( t1coeff, pinmaskD, pinmaskC ); // ???
		ReadBitRVSWD( t1coeff, pinmaskD, pinmaskC ); // ???
		ReadBitRVSWD( t1coeff, pinmaskD, pinmaskC ); // ???
		SendBitRVSWD( t1coeff, pinmaskD, pinmaskC, 1 ); // 1 for data
		SendBitRVSWD( t1coeff, pinmaskD, pinmaskC, 0 ); // ??? Seems to have something to do with halting?

		GPIO.out_w1tc = pinmaskC;
		PrecDelay( t1coeff );
		GPIO.out_w1tc = pinmaskD;
		GPIO.enable_w1ts = pinmaskD;
		PrecDelay( t1coeff );
		GPIO.out_w1ts = pinmaskC;
		PrecDelay( t1coeff );
		GPIO.out_w1ts = pinmaskD;

		EnableISR();
		esp_rom_delay_us(8); // Sometimes 2 is too short.
	}
	return 0;
}

#endif // RUNNING_ON_ESP32S2

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// High level functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int InitializeSWDSWIO( struct SWIOState * state )
{
	for( int timeout = 20; timeout; timeout-- )
	{
#ifndef RUNNING_ON_ESP32S2
		//printf( "CFG FOR RV\n" );
		ConfigureIOForRVSWIO();
		//printf( "DONE CFG FOR RV\n" );
#endif
		// Careful - don't halt the part, we might just want to attach for a debug printf or something.
		state->target_chip_type = CHIP_UNKNOWN;
		state->sectorsize = 64;

		state->opmode = 1; // Try SWIO first
		// First try to see if there is an 003.
		MCFWriteReg32( state, DMSHDWCFGR, 0x5aa50000 | (1<<10) ); // Shadow Config Reg
		MCFWriteReg32( state, DMCFGR, 0x5aa50000 | (1<<10) ); // CFGR (1<<10 == Allow output from slave)
		MCFWriteReg32( state, DMSHDWCFGR, 0x5aa50000 | (1<<10) ); // Try twice just in case
		MCFWriteReg32( state, DMCFGR, 0x5aa50000 | (1<<10) );

		MCFWriteReg32( state, DMCONTROL, 0x00000001 );
		MCFWriteReg32( state, DMCONTROL, 0x00000001 );

		// See if we can see a chip here...
		uint32_t value = 0;
		int readdm = MCFReadReg32( state, DMCFGR, &value );
		if( readdm == 0 && ( value & 0xffff0000 ) == ( 0x5aa50000 ) )
		{
			BB_PRINTF_DEBUG( "Found RVSWIO interface.\n" );
			return 0;
		}

#if RUNNING_ON_ESP32S2
		GPIO.out_w1ts = state->pinmaskC;
		GPIO.enable_w1ts = state->pinmaskC;
#else
		//printf( "CFG FOR SWD\n" );
		ConfigureIOForRVSWD();
		//printf( "DONE CFG FOR SWD\n" );
#endif

		//Otherwise Maybe it's SWD?
		state->opmode = 2;

		MCFWriteReg32( state, DMCONTROL, 0x00000001 );
		MCFWriteReg32( state, DMCONTROL, 0x00000001 );

		uint32_t dmstatus, dmcontrol;
		if( MCFReadReg32( state, DMSTATUS, &dmstatus ) != 0 || 
			MCFReadReg32( state, DMCONTROL, &dmcontrol ) != 0 )
		{
			//BB_PRINTF_DEBUG( "Could not read from RVSWD connection\n" );
			state->opmode = 0;
			continue;
		}

		// See if we can see a chip here...
		if( ( ( ( dmstatus >> 8 ) & 0xf ) != 0x0c &&
			( ( dmstatus >> 8 ) & 0xf ) != 0x03 ) ||
			dmcontrol != 1 )
		{
			//BB_PRINTF_DEBUG( "DMSTATUS invalid (Probably no RVSWD chip)\n" );
			state->opmode = 0;
			continue;
		}

		MCFWriteReg32( state, DMABSTRACTCS, 0x08000302 ); // Clear out abstractcs register.
		BB_PRINTF_DEBUG( "Found RVSWD interface\n" );
		return 0;
	}
	//printf( "TIMEOUT\n" );
	return -55;
}

static int DetermineChipTypeAndSectorInfo( struct SWIOState * iss )
{
	if( iss->target_chip_type == CHIP_UNKNOWN )
	{
		if( iss->opmode == 0 )
		{
			int r = InitializeSWDSWIO( iss );
			if( r ) return r;
		}
		uint32_t rr;
		if( MCFReadReg32( iss, DMHARTINFO, &rr ) )
		{
			BB_PRINTF_DEBUG( "Error: Could not get hart info.\n" );
			return -1;
		}

		uint32_t data0offset = 0xe0000000 | ( rr & 0x7ff );

		MCFWriteReg32( iss, DMCONTROL, 0x80000001 ); // Make the debug module work properly.
		MCFWriteReg32( iss, DMCONTROL, 0x80000001 ); // Make the debug module work properly.

		// Tricky, this function needs to clean everything up because it may be used entering debugger.
		uint32_t old_data0;
		MCFReadReg32( iss, BDMDATA0, &old_data0 );
		MCFWriteReg32( iss, DMCOMMAND, 0x00221008 );		// Copy data from x8.
		uint32_t old_x8;
		MCFReadReg32( iss, BDMDATA0, &old_x8 );

		uint32_t vendorid = 0;
		uint32_t marchid = 0;

		MCFWriteReg32( iss, DMABSTRACTCS, 0x08000700 ); // Clear out any dmabstractcs errors.

		MCFWriteReg32( iss, DMABSTRACTAUTO, 0x00000000 );
		MCFWriteReg32( iss, DMCOMMAND, 0x00220000 | 0xf12 );
		MCFWriteReg32( iss, DMCOMMAND, 0x00220000 | 0xf12 );  // Need to double-read, not sure why.
		MCFReadReg32( iss, BDMDATA0, &marchid );

		MCFWriteReg32( iss, DMPROGBUF0, 0x90024000 );		// c.ebreak <<== c.lw x8, 0(x8)
		MCFWriteReg32( iss, BDMDATA0, 0x1ffff704 );			// Special chip ID location.
		MCFWriteReg32( iss, DMCOMMAND, 0x00271008 );		// Copy data to x8, and execute.

		uint32_t abstractcscheck = 0;
		MCFReadReg32( iss, DMABSTRACTCS, &abstractcscheck );

		WaitForDoneOp( iss ); //Doesn't need to actually do this, but we can wait here.

		MCFWriteReg32( iss, DMCOMMAND, 0x00221008 );		// Copy data from x8.
		MCFReadReg32( iss, BDMDATA0, &vendorid );

		// Cleanup
		MCFWriteReg32( iss, BDMDATA0, old_x8 );
		MCFWriteReg32( iss, DMCOMMAND, 0x00231008 );		// Copy data to x8
		MCFWriteReg32( iss, BDMDATA0, old_data0 );

		if( data0offset == 0xe00000f4 )
		{
			// Only known processor with this signature = 0 is a CH32V003.
			switch( vendorid >> 20 )
			{
			case 0x002: iss->target_chip_type = CHIP_CH32V002; iss->sectorsize = 256; break;
			case 0x004: iss->target_chip_type = CHIP_CH32V004; iss->sectorsize = 256; break;
			case 0x005: iss->target_chip_type = CHIP_CH32V005; iss->sectorsize = 256; break;
			case 0x006: iss->target_chip_type = CHIP_CH32V006; iss->sectorsize = 256; break;
			default:    iss->target_chip_type = CHIP_CH32V003; iss->sectorsize = 64; break; // not usually 003
			}
			// Examples:
			// 00000012 = CHIP_CH32V003
		}
		else if( data0offset == 0xe0000380 )
		{
			// All other known chips.
			uint32_t chip_type = (vendorid & 0xff000000)>>24;
			switch( chip_type )
			{
				case 0x10:
					iss->target_chip_type = CHIP_CH32V10x;
					iss->sectorsize = 256; // test me!
					break;
				case 0x03: // x03x
					iss->target_chip_type = CHIP_CH32X03x;
					iss->sectorsize = 256;  // Should be 128, but, doesn't work with 128, only 256
					break;
				case 0x20: // v20x
					iss->target_chip_type = CHIP_CH32V20x;
					iss->sectorsize = 256;
					break;
				case 0x30:
					iss->target_chip_type = CHIP_CH32V30x;
					iss->sectorsize = 256;
					break;
			}
		}
		//BB_PRINTF_DEBUG( "Detected %d / %d\n", iss->target_chip_type, iss->sectorsize );
		//BB_PRINTF_DEBUG( "Vendored: %08x\n", (unsigned)vendorid );
		//BB_PRINTF_DEBUG( "marchid : %08x\n", (unsigned)marchid );
		//BB_PRINTF_DEBUG( "HARTINFO: %08x\n", (unsigned)rr );

		iss->statetag = STTAG( "XXXX" );
		if( !iss->target_chip_type ) return -5;
	}
	return 0;
}

static int WaitForFlash( struct SWIOState * iss )
{
	struct SWIOState * dev = iss;

	if( DetermineChipTypeAndSectorInfo( iss ) ) return -9;

	uint32_t rw, timeout = 0;
	do
	{
		rw = 0;
		ReadWord( dev, 0x4002200C, &rw ); // FLASH_STATR => 0x4002200C
	} while( (rw & 1) && timeout++ < 2000);  // BSY flag.

	WriteWord( dev, 0x4002200C, 0 );

	if( rw & FLASH_STATR_WRPRTERR )
		return -44;

	if( rw & 1 )
		return -5;

	return 0;
}


static int WaitForDoneOp( struct SWIOState * iss )
{
	int r;
	uint32_t rrv;
	int ret = 0;
	int timeout = 1000;
	struct SWIOState * dev = iss;
	do
	{
		r = MCFReadReg32( dev, DMABSTRACTCS, &rrv );
		if( r ) return r;
		if( timeout-- == 0 ) return -8;
	}
	while( rrv & (1<<12) );
	if( (rrv >> 8 ) & 7 )
	{
		MCFWriteReg32( dev, DMABSTRACTCS, 0x00000700 );
		ret = -33;
	}
	return ret;
}

static int StaticUpdatePROGBUFRegs( struct SWIOState * dev )
{
	if( DetermineChipTypeAndSectorInfo( dev ) ) return -9;

	MCFWriteReg32( dev, DMABSTRACTAUTO, 0 ); // Disable Autoexec.
	uint32_t rr;
	MCFReadReg32( dev, DMHARTINFO, &rr );
	uint32_t data0offset = 0xe0000000 | ( rr & 0x7ff );
	MCFWriteReg32( dev, BDMDATA0, data0offset );   // DATA0's location in memory. (hard code to 0xe00000f4 if only working on the 003)
	MCFWriteReg32( dev, DMCOMMAND, 0x0023100a ); // Copy data to x10
	MCFWriteReg32( dev, BDMDATA0, data0offset + 4 );   // DATA1's location in memory. (hard code to 0xe00000f8 if only working on the 003)
	MCFWriteReg32( dev, DMCOMMAND, 0x0023100b ); // Copy data to x11
	MCFWriteReg32( dev, BDMDATA0, 0x4002200c ); //FLASH->STATR (note add 4 to FLASH->CTLR)
	MCFWriteReg32( dev, DMCOMMAND, 0x0023100c ); // Copy data to x12

	// This is not even needed on the v20x/v30x chips.  But it won't harm us to set the register for simplicity.
	// v003 requires bufload every word.
	// x035 requires bufload every word in spite of what the datasheet says.
	// CR_PAGE_PG = FTPG = 0x00010000 | CR_BUF_LOAD = 0x00040000
	MCFWriteReg32( dev, BDMDATA0, 0x00010000|0x00040000 );

	MCFWriteReg32( dev, DMCOMMAND, 0x0023100d ); // Copy data to x13
	return 0;
}

static void ResetInternalProgrammingState( struct SWIOState * iss )
{
	iss->statetag = 0;
	iss->lastwriteflags = 0;
	iss->currentstateval = 0;
	iss->flash_unlocked = 0;
	iss->autoincrement = 0;
}

static int ReadWord( struct SWIOState * iss, uint32_t address_to_read, uint32_t * data )
{
	struct SWIOState * dev = iss;
	int autoincrement = 1;

	if( address_to_read == 0x40022010 || address_to_read == 0x4002200C )  // Don't autoincrement when checking flash flag. 
		autoincrement = 0;

	if( iss->statetag != STTAG( "RDSQ" ) || address_to_read != iss->currentstateval || autoincrement != iss->autoincrement )
	{
		if( iss->statetag != STTAG( "RDSQ" ) || autoincrement != iss->autoincrement )
		{
			if( iss->statetag != STTAG( "WRSQ" ) )
			{
				int r = StaticUpdatePROGBUFRegs( dev );
				if( r ) return r;
			}

			MCFWriteReg32( dev, DMABSTRACTAUTO, 0 ); // Disable Autoexec.

			// c.lw x8,0(x11) // Pull the address from DATA1
			// c.lw x9,0(x8)  // Read the data at that location.
			MCFWriteReg32( dev, DMPROGBUF0, 0x40044180 );
			if( autoincrement )
			{
				// c.addi x8, 4
				// c.sw x9, 0(x10) // Write back to DATA0

				MCFWriteReg32( dev, DMPROGBUF1, 0xc1040411 );
			}
			else
			{
				// c.nop
				// c.sw x9, 0(x10) // Write back to DATA0

				MCFWriteReg32( dev, DMPROGBUF1, 0xc1040001 );
			}
			// c.sw x8, 0(x11) // Write addy to DATA1
			// c.ebreak
			MCFWriteReg32( dev, DMPROGBUF2, 0x9002c180 );
			MCFWriteReg32( dev, DMABSTRACTAUTO, 1 ); // Enable Autoexec (not autoincrement)
			iss->autoincrement = autoincrement;
		}

		MCFWriteReg32( dev, BDMDATA1, address_to_read );
		MCFWriteReg32( dev, DMCOMMAND, 0x00240000 ); // Execute.

		iss->statetag = STTAG( "RDSQ" );
		iss->currentstateval = address_to_read;
	}

	if( iss->autoincrement )
		iss->currentstateval += 4;

	// Only an issue if we are curising along very fast.
	int r = WaitForDoneOp( dev );
	if( r ) return r;

	r = MCFReadReg32( dev, BDMDATA0, data );
	return r;
}

static int WriteWord( struct SWIOState * iss, uint32_t address_to_write, uint32_t data )
{
	struct SWIOState * dev = iss;

	int ret = 0;

	int is_flash = 0;
	if( ( address_to_write & 0xff000000 ) == 0x08000000 || ( address_to_write & 0x1FFF0000 ) == 0x1FFF0000 )
	{
		// Is flash.
		is_flash = 1;
	}

	if( iss->statetag != STTAG( "WRSQ" ) || is_flash != iss->lastwriteflags )
	{
		int did_disable_req = 0;

		if( iss->statetag != STTAG( "WRSQ" ) )
		{
			MCFWriteReg32( dev, DMABSTRACTAUTO, 0x00000000 ); // Disable Autoexec.
			did_disable_req = 1;

			if( iss->statetag != STTAG( "RDSQ" ) )
			{
				int r = StaticUpdatePROGBUFRegs( dev );
				if( r ) return r;
			}

			// Different address, so we don't need to re-write all the program regs.
			// c.lw x8,0(x10) // Get the value to write.
			// c.lw x9,0(x11) // Get the address to write to. 
			MCFWriteReg32( dev, DMPROGBUF0, 0x41844100 );
			// c.sw x8,0(x9)  // Write to the address.
			// c.addi x9, 4
			MCFWriteReg32( dev, DMPROGBUF1, 0x0491c080 );
			// c.sw x9,0(x11)
			// c.nop
			MCFWriteReg32( dev, DMPROGBUF2, 0x0001c184 );
			// We don't shorthand the stop here, because if we are flipping beteen flash and
			// non-flash writes, we don't want to keep messing with these registers.
		}

		if( is_flash )
		{
			// A little weird - we need to wait until the buf load is done here to continue.
			// x12 = 0x40022010 (FLASH_STATR)
			//
			// c254 c.sw x13,4(x12) // Acknowledge the page write.  (BUT ONLY ON x035 / v003)
			//  otherwise c.nop
			// 4200 c.lw x8,0(x12)  // Start checking to see when buf load is done.
			// 8809 c.andi x8, 2    // Only look at WR_BSY (seems to be rather undocumented)
			//  8805 c.andi x8, 1    // Only look at BSY if we're not on a v30x / v20x
			// fc75 c.bnez x8, -4
			// c.ebreak
			MCFWriteReg32( dev, DMPROGBUF3, 
				(iss->target_chip_type == CHIP_CH32X03x || iss->target_chip_type == CHIP_CH32V003 || (iss->target_chip_type >= CHIP_CH32V002 && iss->target_chip_type <= CHIP_CH32V006 ) ) ? 
				0x4200c254 : 0x42000001  );

			MCFWriteReg32( dev, DMPROGBUF4,
				(iss->target_chip_type == CHIP_CH32V20x || iss->target_chip_type == CHIP_CH32V30x ) ?
				0xfc758809 : 0xfc758805 );

			MCFWriteReg32( dev, DMPROGBUF5, 0x90029002 );
		}
		else
		{
			MCFWriteReg32( dev, DMPROGBUF3, 0x90029002 ); // c.ebreak (nothing needs to be done if not flash)
		}

		MCFWriteReg32( dev, BDMDATA1, address_to_write );
		MCFWriteReg32( dev, BDMDATA0, data );

		if( did_disable_req )
		{
			MCFWriteReg32( dev, DMCOMMAND, 0x00240000 ); // Execute.
			MCFWriteReg32( dev, DMABSTRACTAUTO, 1 ); // Enable Autoexec.
		}

		iss->lastwriteflags = is_flash;

		iss->statetag = STTAG( "WRSQ" );
		iss->currentstateval = address_to_write;
	}
	else
	{
		if( address_to_write != iss->currentstateval )
		{
			MCFWriteReg32( dev, BDMDATA1, address_to_write );
		}
		MCFWriteReg32( dev, BDMDATA0, data );
	}
	if( is_flash )
		ret |= WaitForDoneOp( dev );

	iss->currentstateval += 4;

	return ret;
}

static int UnlockFlash( struct SWIOState * iss )
{
	struct SWIOState * dev = iss;

	if( DetermineChipTypeAndSectorInfo( iss ) ) return -9;

	uint32_t rw;
	ReadWord( dev, 0x40022010, &rw );  // FLASH->CTLR = 0x40022010
	if( rw & 0x8080 ) 
	{

		WriteWord( dev, 0x40022004, 0x45670123 ); // FLASH->KEYR = 0x40022004
		WriteWord( dev, 0x40022004, 0xCDEF89AB );
		WriteWord( dev, 0x40022008, 0x45670123 ); // OBKEYR = 0x40022008
		WriteWord( dev, 0x40022008, 0xCDEF89AB );
		WriteWord( dev, 0x40022024, 0x45670123 ); // MODEKEYR = 0x40022024
		WriteWord( dev, 0x40022024, 0xCDEF89AB );

		ReadWord( dev, 0x40022010, &rw ); // FLASH->CTLR = 0x40022010
		if( rw & 0x8080 ) 
		{
			return -9;
		}
	}
	iss->statetag = STTAG( "XXXX" );
	iss->flash_unlocked = 1;
	return 0;
}

static int EraseFlash( struct SWIOState * iss, uint32_t address, uint32_t length, int type )
{
	struct SWIOState * dev = iss;

	uint32_t rw;

	if( !iss->flash_unlocked )
	{
		if( ( rw = UnlockFlash( iss ) ) )
			return rw;
	}

	if( type == 1 )
	{
		// Whole-chip flash
		iss->statetag = STTAG( "XXXX" );
		BB_PRINTF_DEBUG( "Whole-chip erase\n" );
		WriteWord( dev, 0x40022010, 0 ); //  FLASH->CTLR = 0x40022010
		WriteWord( dev, 0x40022010, FLASH_CTLR_MER  );
		WriteWord( dev, 0x40022010, CR_STRT_Set|FLASH_CTLR_MER );
		if( WaitForFlash( dev ) ) return -13;
		WriteWord( dev, 0x40022010, 0 ); //  FLASH->CTLR = 0x40022010
	}
	else
	{
		// 16.4.7, Step 3: Check the BSY bit of the FLASH_STATR register to confirm that there are no other programming operations in progress.
		// skip (we make sure at the end)

		int chunk_to_erase = address;

		while( chunk_to_erase < address + length )
		{
			if( WaitForFlash( dev ) ) return -14;

			// Step 4:  set PAGE_ER of FLASH_CTLR(0x40022010)
			WriteWord( dev, 0x40022010, CR_PAGE_ER ); // Actually FTER //  FLASH->CTLR = 0x40022010

			// Step 5: Write the first address of the fast erase page to the FLASH_ADDR register.
			WriteWord( dev, 0x40022014, chunk_to_erase  ); // FLASH->ADDR = 0x40022014

			// Step 6: Set the STAT bit of FLASH_CTLR register to '1' to initiate a fast page erase (64 bytes) action.
			WriteWord( dev, 0x40022010, CR_STRT_Set|CR_PAGE_ER );  // FLASH->CTLR = 0x40022010
			if( WaitForFlash( dev ) ) return -15;

			WriteWord( dev, 0x40022010, 0 ); //  FLASH->CTLR = 0x40022010 (Disable any pending ops)
			chunk_to_erase+=64;
		}
	}
	return 0;
}


static int Write64Block( struct SWIOState * iss, uint32_t address_to_write, uint8_t * blob )
{
	struct SWIOState * dev = iss;

	if( DetermineChipTypeAndSectorInfo( iss ) ) return -9;

	const int blob_size = 64;
	uint32_t wp = address_to_write;
	uint32_t ew = wp + blob_size;
	int group = -1;
	int is_flash = 0;
	int rw = 0;

	if( (address_to_write & 0xff000000) == 0x08000000 || (address_to_write & 0xff000000) == 0x00000000  || (address_to_write & 0x1FFF0000) == 0x1FFF0000  ) 
	{
		// Need to unlock flash.
		// Flash reg base = 0x40022000,
		// FLASH_MODEKEYR => 0x40022024
		// FLASH_KEYR => 0x40022004

		if( !iss->flash_unlocked )
		{
			if( ( rw = UnlockFlash( dev ) ) )
				return rw;
		}

		is_flash = 1;

		// Only erase on first block in sector.
		int block_in_sector = (wp & ( iss->sectorsize - 1 )) / blob_size;
		int is_first_block = block_in_sector == 0;

		if( is_first_block )
		{
			rw = EraseFlash( dev, address_to_write, blob_size, 0 );
			if( rw ) return rw;
			// 16.4.6 Main memory fast programming, Step 5
			//if( WaitForFlash( dev ) ) return -11;
			//WriteWord( dev, 0x40022010, FLASH_CTLR_BUF_RST );
			//if( WaitForFlash( dev ) ) return -11;
		}
	}

	/* General Note:
		Most flash operations take about 3ms to complete :(
	*/

	while( wp < ew )
	{
		if( is_flash )
		{
			group = (wp & 0xffffffc0);

			int block_in_sector = (group & ( iss->sectorsize - 1 )) / blob_size;
			int is_first_block = block_in_sector == 0;
			int is_last_block = block_in_sector == (iss->sectorsize / blob_size - 1 );

			if( is_first_block )
			{
				if( dev->target_chip_type == CHIP_CH32V20x || dev->target_chip_type == CHIP_CH32V30x )
				{
					// No bufrst on v20x, v30x
					WaitForFlash( dev );
					WriteWord( dev, 0x40022010, CR_PAGE_PG ); // THIS IS REQUIRED, (intptr_t)&FLASH->CTLR = 0x40022010
					//FTPG ==  CR_PAGE_PG   == ((uint32_t)0x00010000)
				}
				else
				{
					// V003, x035, maybe more.
					WriteWord( dev, 0x40022010, CR_PAGE_PG ); // THIS IS REQUIRED, (intptr_t)&FLASH->CTLR = 0x40022010
					WriteWord( dev, 0x40022010, CR_BUF_RST | CR_PAGE_PG );  // (intptr_t)&FLASH->CTLR = 0x40022010
				}
				WaitForFlash( dev );
			}

			int j;
			for( j = 0; j < 16; j++ )
			{
				int index = (wp-address_to_write);
				uint32_t data = 0xffffffff;
				if( index + 3 < blob_size )
				{
					memcpy( &data, &blob[index], 4 );
				}
				else if( (int32_t)(blob_size - index) > 0 )
				{
					memcpy( &data, &blob[index], blob_size - index );
				}
				WriteWord( dev, wp, data );
				wp += 4;
			}


			if( ( iss->target_chip_type == CHIP_CH32V20x || iss->target_chip_type == CHIP_CH32V30x ) )
			{
				if( is_last_block )
				{
					WriteWord( dev, 0x40022010, CR_PAGE_PG | (1<<21) ); // Page Start
					if( WaitForFlash( dev ) ) return -13;
				}
			}
			else // TODO: What about the v10x?
			{
				// Datasheet says the x03x needs to have this called every group-of-16, but that's not true, it should be every 16-words.

				WriteWord( dev, 0x40022014, group );  //0x40022014 -> FLASH->ADDR
				//if( MCF.PrepForLongOp ) MCF.PrepForLongOp( dev );  // Give the programmer a headsup this next operation could take a while.
				WriteWord( dev, 0x40022010, CR_PAGE_PG|CR_STRT_Set ); // 0x40022010 -> FLASH->CTLR
				if( WaitForFlash( dev ) ) return -16;
			}
		}
		else
		{
			int index = (wp-address_to_write);
			uint32_t data = 0xffffffff;
			if( index + 3 < blob_size )
			{
				memcpy( &data, &blob[index], 4 );
			}
			else if( (int32_t)(blob_size - index) > 0 )
			{
				memcpy( &data, &blob[index], blob_size - index );
			}
			WriteWord( dev, wp, data );
			wp += 4;
		}
	}

	return 0;
}

// Polls up to 7 bytes of printf, and can leave a 7-bit flag for the CH32V003.
static int PollTerminal( struct SWIOState * iss, uint8_t * buffer, int maxlen, uint32_t leavevalA, uint32_t leavevalB )
{
	struct SWIOState * dev = iss;

	int r;
	uint32_t rr;
	if( iss->statetag != STTAG( "TERM" ) )
	{
		MCFWriteReg32( dev, DMABSTRACTAUTO, 0x00000000 ); // Disable Autoexec.
		iss->statetag = STTAG( "TERM" );
	}
	r = MCFReadReg32( dev, BDMDATA0, &rr );
	if( r < 0 ) return r;

	if( maxlen < 8 ) return -9;

	// BDMDATA1:
	//  bit  7 = host-acknowledge.
	if( rr & 0x80 )
	{
		int ret = 0;
		int num_printf_chars = (rr & 0xf)-4;

		if( num_printf_chars > 0 && num_printf_chars <= 7)
		{
			if( num_printf_chars > 3 )
			{
				uint32_t r2;
				r = MCFReadReg32( dev, BDMDATA1, &r2 );
				memcpy( buffer+3, &r2, num_printf_chars - 3 );
			}
			int firstrem = num_printf_chars;
			if( firstrem > 3 ) firstrem = 3;
			memcpy( buffer, ((uint8_t*)&rr)+1, firstrem );
			buffer[num_printf_chars] = 0;
			ret = num_printf_chars;
		}
		if( leavevalA )
		{
			MCFWriteReg32( dev, BDMDATA1, leavevalB );
		}
		MCFWriteReg32( dev, BDMDATA0, leavevalA ); // Write that we acknowledge the data.
		return ret;
	}
	else
	{
		return 0;
	}
}

#endif // _CH32V003_SWIO_H

