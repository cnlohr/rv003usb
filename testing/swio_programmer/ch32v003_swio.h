// CH32V003 SWIO minimal reference implementation for bit banged IO
// on the ESP32-S2.
//
// Copyright 2023 Charles Lohr, May be licensed under the MIT/x11 or NewBSD
// licenses.  You choose.  (Can be included in commercial and copyleft work)
//
// This file is original work.
//
// Mostly tested, though, not perfect.  Expect to tweak some things.
// DoSongAndDanceToEnterPgmMode is almost completely untested.
// This is the weird song-and-dance that the WCH LinkE does when
// connecting to a CH32V003 part with unknown state.  This is probably
// incorrect, but isn't really needed unless things get really cursed.

#ifndef _CH32V003_SWIO_H
#define _CH32V003_SWIO_H

// This is a hacky thing, but if you are laaaaazzzyyyy and don't want to add a 10k
// resistor, youcan do this.  It glitches the line high very, very briefly.  
// Enable for when you don't have a 10k pull-upand are relying on the internal pull-up.
// WARNING: If you set this, you should set the drive current to 5mA.
#define R_GLITCH_HIGH


#ifndef INCLUDE_RAW_IO
#define INCLUDE_RAW_IO 0
#endif

// You should interface to this file via these functions
struct SWIOState
{
#if INCLUDE_RAW_IO
	// Set these before calling any functions
	int t1coeff;
	int pinmask;
#endif

	// Zero the rest of the structure.
	uint32_t statetag;
	uint32_t lastwriteflags;
	uint32_t currentstateval;
	uint32_t flash_unlocked;
	uint32_t autoincrement;
};

#define STTAG( x ) (*((uint32_t*)(x)))

#define IRAM IRAM_ATTR

//static int DoSongAndDanceToEnterPgmMode( struct SWIOState * state );
static void MCFWriteReg32( struct SWIOState * state, uint8_t command, uint32_t value ) IRAM;
static int MCFReadReg32( struct SWIOState * state, uint8_t command, uint32_t * value ) IRAM;

// More advanced functions built on lower level PHY.
static int ReadWord( struct SWIOState * state, uint32_t word, uint32_t * ret );
static int WriteWord( struct SWIOState * state, uint32_t word, uint32_t val );
static int WaitForFlash( struct SWIOState * state );
static int WaitForDoneOp( struct SWIOState * state );
static int Write64Block( struct SWIOState * iss, uint32_t address_to_write, uint8_t * data );
static int UnlockFlash( struct SWIOState * iss );
static int EraseFlash( struct SWIOState * iss, uint32_t address, uint32_t length, int type );
static void ResetInternalProgrammingState( struct SWIOState * iss );
static int PollTerminal( struct SWIOState * iss, uint8_t * buffer, int maxlen, uint32_t leavevalA, uint32_t leavevalB );


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


#define DMDATA0R       0x04
#define DMDATA1R       0x05

#define DMCPBR       0x7C
#define DMCFGR       0x7D
#define DMSHDWCFGR   0x7E

#ifndef FLASH_STATR_WRPRTERR
#define FLASH_STATR_WRPRTERR       ((uint8_t)0x10) 
#define CR_PAGE_PG                 ((uint32_t)0x00010000)
#define CR_BUF_LOAD                ((uint32_t)0x00040000)
#define FLASH_CTLR_MER             ((uint16_t)0x0004)     /* Mass Erase */
#define CR_STRT_Set                ((uint32_t)0x00000040)
#define CR_PAGE_ER                 ((uint32_t)0x00020000)
#define CR_BUF_RST                 ((uint32_t)0x00080000)
#endif

#if INCLUDE_RAW_IO

static inline void Send1Bit( int t1coeff, int pinmask ) IRAM;
static inline void Send0Bit( int t1coeff, int pinmask ) IRAM;
static inline int ReadBit( struct SWIOState * state ) IRAM;


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
static inline void Send1Bit( int t1coeff, int pinmask )
{
	// Low for a nominal period of time.
	// High for a nominal period of time.

	GPIO.out_w1tc = pinmask;
	PrecDelay( t1coeff );
	GPIO.out_w1ts = pinmask;
	PrecDelay( t1coeff );
}

static inline void Send0Bit( int t1coeff, int pinmask )
{
	// Low for a LONG period of time.
	// High for a nominal period of time.
	int longwait = t1coeff*4;
	GPIO.out_w1tc = pinmask;
	PrecDelay( longwait );
	GPIO.out_w1ts = pinmask;
	PrecDelay( t1coeff );
}

// returns 0 if 0
// returns 1 if 1
// returns 2 if timeout.
static inline int ReadBit( struct SWIOState * state )
{
	int t1coeff = state->t1coeff;
	int pinmask = state->pinmask;

	// Drive low, very briefly.  Let drift high.
	// See if CH32V003 is holding low.

	int timeout = 0;
	int ret = 0;
	int medwait = t1coeff * 2;
	GPIO.out_w1tc = pinmask;
	PrecDelay( t1coeff );
	GPIO.enable_w1tc = pinmask;
	GPIO.out_w1ts = pinmask;
#ifdef R_GLITCH_HIGH
	int halfwait = t1coeff / 2;
	PrecDelay( halfwait );
	GPIO.enable_w1ts = pinmask;
	GPIO.enable_w1tc = pinmask;
	PrecDelay( halfwait );
#else
	PrecDelay( medwait );
#endif
	ret = GPIO.in;

#ifdef R_GLITCH_HIGH
	if( !(ret & pinmask) )
	{
		// Wait if still low.
		PrecDelay( medwait );
		GPIO.enable_w1ts = pinmask;
		GPIO.enable_w1tc = pinmask;
	}
#endif
	for( timeout = 0; timeout < MAX_IN_TIMEOUT; timeout++ )
	{
		if( GPIO.in & pinmask )
		{
			GPIO.enable_w1ts = pinmask;
			int fastwait = t1coeff / 2;
			PrecDelay( fastwait );
			return !!(ret & pinmask);
		}
	}
	
	// Force high anyway so, though hazarded, we can still move along.
	GPIO.enable_w1ts = pinmask;
	return 2;
}

static void MCFWriteReg32( struct SWIOState * state, uint8_t command, uint32_t value )
{
	int t1coeff = state->t1coeff;
	int pinmask = state->pinmask;

 	GPIO.out_w1ts = pinmask;
	GPIO.enable_w1ts = pinmask;

	DisableISR();
	Send1Bit( t1coeff, pinmask );
	uint32_t mask;
	for( mask = 1<<6; mask; mask >>= 1 )
	{
		if( command & mask )
			Send1Bit(t1coeff, pinmask);
		else
			Send0Bit(t1coeff, pinmask);
	}
	Send1Bit( t1coeff, pinmask );
	for( mask = 1<<31; mask; mask >>= 1 )
	{
		if( value & mask )
			Send1Bit(t1coeff, pinmask);
		else
			Send0Bit(t1coeff, pinmask);
	}
	EnableISR();
	esp_rom_delay_us(8); // Sometimes 2 is too short.
}

// returns 0 if no error, otherwise error.
static int MCFReadReg32( struct SWIOState * state, uint8_t command, uint32_t * value )
{
	int t1coeff = state->t1coeff;
	int pinmask = state->pinmask;

 	GPIO.out_w1ts = pinmask;
	GPIO.enable_w1ts = pinmask;

	DisableISR();
	Send1Bit( t1coeff, pinmask );
	int i;
	uint32_t mask;
	for( mask = 1<<6; mask; mask >>= 1 )
	{
		if( command & mask )
			Send1Bit(t1coeff, pinmask);
		else
			Send0Bit(t1coeff, pinmask);
	}
	Send0Bit( t1coeff, pinmask );
	uint32_t rval = 0;
	for( i = 0; i < 32; i++ )
	{
		rval <<= 1;
		int r = ReadBit( state );
		if( r == 1 )
			rval |= 1;
		if( r == 2 )
		{
			EnableISR();
			return -1;
		}
	}
	*value = rval;
	EnableISR();
	esp_rom_delay_us(8); // Sometimes 2 is too short.
	return 0;
}

static inline void ExecuteTimePairs( struct SWIOState * state, const uint16_t * pairs, int numpairs, int iterations )
{
	int t1coeff = state->t1coeff;
	int pinmask = state->pinmask;

	int j, k;
	for( k = 0; k < iterations; k++ )
	{
		const uint16_t * tp = pairs;
		for( j = 0; j < numpairs; j++ )
		{
			int t1v = t1coeff * (*(tp++))-1;
			GPIO.out_w1tc = pinmask;
			PrecDelay( t1v );
			GPIO.out_w1ts = pinmask;
			t1v = t1coeff * (*(tp++))-1;
			PrecDelay( t1v );
		}
	}
}


#endif

#if 0
// Returns 0 if chips is present
// Returns 1 if chip is not present
// Returns 2 if there was a bus fault.
static int DoSongAndDanceToEnterPgmMode( struct SWIOState * state )
{
	int pinmask = state->pinmask;
	// XXX MOSTLY UNTESTED!!!!

	static const uint16_t timepairs1[] ={
		32, 12, //  8.2us / 3.1us
		36, 12, //  9.2us / 3.1us
		392, 366 // 102.3us / 95.3us
	};

	// Repeat for 199x
	static const uint16_t timepairs2[] ={
		15, 12, //  4.1us / 3.1us
		32, 12, //  8.2us / 3.1us
		36, 12, //  9.3us / 3.1us
		392, 366 // 102.3us / 95.3us
	};

	// Repeat for 10x
	static const uint16_t timepairs3[] ={
		15, 807, //  4.1us / 210us
		24, 8,   //  6.3us / 2us
		32, 8,   //  8.4us / 2us
		24, 10,  //  6.2us / 2.4us
		20, 8,   //  5.2us / 2.1us
		239, 8,  //  62.3us / 2.1us
		32, 20,  //  8.4us / 5.3us
		8, 32,   //  2.2us / 8.4us
		24, 8,   //  6.3us / 2.1us
		32, 8,   //  8.4us / 2.1us
		26, 7,   //  6.9us / 1.7us
		20, 8,   //  5.2us / 2.1us
		239, 8,  //  62.3us / 2.1us
		32, 20,  //  8.4us / 5.3us
		8, 22,   //  2us / 5.3us
		24, 8,   //  6.3us 2.1us
		24, 8,   //  6.3us 2.1us
		31, 6,   //  8us / 1.6us
		25, 307, // 6.6us / 80us
	};

	DisableISR();

	ExecuteTimePairs( state, timepairs1, 3, 1 );
	ExecuteTimePairs( state, timepairs2, 4, 199 );
	ExecuteTimePairs( state, timepairs3, 19, 10 );

	// THIS IS WRONG!!!! THIS IS NOT A PRESENT BIT. 
	int present = ReadBit( state ); // Actually here t1coeff, for this should be *= 8!
	GPIO.enable_w1ts = pinmask;
	GPIO.out_w1tc = pinmask;
	esp_rom_delay_us( 2000 );
	GPIO.out_w1ts = pinmask;
	EnableISR();
	esp_rom_delay_us( 1 );

	return present;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Higher level functions

static int WaitForFlash( struct SWIOState * iss )
{
	struct SWIOState * dev = iss;
	uint32_t rw, timeout = 0;
	do
	{
		rw = 0;
		ReadWord( dev, 0x4002200C, &rw ); // FLASH_STATR => 0x4002200C
	} while( (rw & 1) && timeout++ < 200);  // BSY flag.

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
	struct SWIOState * dev = iss;
	do
	{
		r = MCFReadReg32( dev, DMABSTRACTCS, &rrv );
		if( r ) return r;
	}
	while( rrv & (1<<12) );
	if( (rrv >> 8 ) & 7 )
	{
		MCFWriteReg32( dev, DMABSTRACTCS, 0x00000700 );
		ret = -33;
	}
	return ret;
}

static void StaticUpdatePROGBUFRegs( struct SWIOState * dev )
{
	MCFWriteReg32( dev, DMDATA0R, 0xe00000f4 );   // DATA0's location in memory.
	MCFWriteReg32( dev, DMCOMMAND, 0x0023100a ); // Copy data to x10
	MCFWriteReg32( dev, DMDATA0R, 0xe00000f8 );   // DATA1's location in memory.
	MCFWriteReg32( dev, DMCOMMAND, 0x0023100b ); // Copy data to x11
	MCFWriteReg32( dev, DMDATA0R, 0x40022010 ); //FLASH->CTLR
	MCFWriteReg32( dev, DMCOMMAND, 0x0023100c ); // Copy data to x12
	MCFWriteReg32( dev, DMDATA0R, CR_PAGE_PG|CR_BUF_LOAD);
	MCFWriteReg32( dev, DMCOMMAND, 0x0023100d ); // Copy data to x13
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

			if( iss->statetag != STTAG( "WRSQ" ) )
			{
				StaticUpdatePROGBUFRegs( dev );
			}
			MCFWriteReg32( dev, DMABSTRACTAUTO, 1 ); // Enable Autoexec (not autoincrement)
			iss->autoincrement = autoincrement;
		}

		MCFWriteReg32( dev, DMDATA1R, address_to_read );
		MCFWriteReg32( dev, DMCOMMAND, 0x00241000 ); // Only execute.

		iss->statetag = STTAG( "RDSQ" );
		iss->currentstateval = address_to_read;

		WaitForDoneOp( dev );
	}

	if( iss->autoincrement )
		iss->currentstateval += 4;

	return MCFReadReg32( dev, DMDATA0R, data );
}

static int WriteWord( struct SWIOState * iss, uint32_t address_to_write, uint32_t data )
{
	struct SWIOState * dev = iss;

	int ret = 0;

	int is_flash = 0;
	if( ( address_to_write & 0xff000000 ) == 0x08000000 || ( address_to_write & 0x1FFFF800 ) == 0x1FFFF000 )
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
			// Different address, so we don't need to re-write all the program regs.
			// c.lw x9,0(x11) // Get the address to write to. 
			// c.sw x8,0(x9)  // Write to the address.
			MCFWriteReg32( dev, DMPROGBUF0, 0xc0804184 );
			// c.addi x9, 4
			// c.sw x9,0(x11)
			MCFWriteReg32( dev, DMPROGBUF1, 0xc1840491 );

			if( iss->statetag != STTAG( "RDSQ" ) )
			{
				StaticUpdatePROGBUFRegs( dev );
			}
		}

		if( iss->lastwriteflags != is_flash || iss->statetag != STTAG( "WRSQ" ) )
		{
			// If we are doing flash, we have to ack, otherwise we don't want to ack.
			if( is_flash )
			{
				// After writing to memory, also hit up page load flag.
				// c.sw x13,0(x12) // Acknowledge the page write.
				// c.ebreak
				MCFWriteReg32( dev, DMPROGBUF2, 0x9002c214 );
			}
			else
			{
				MCFWriteReg32( dev, DMPROGBUF2, 0x00019002 ); // c.ebreak
			}
		}

		MCFWriteReg32( dev, DMDATA1R, address_to_write );
		MCFWriteReg32( dev, DMDATA0R, data );

		if( did_disable_req )
		{
			MCFWriteReg32( dev, DMCOMMAND, 0x00271008 ); // Copy data to x8, and execute program.
			MCFWriteReg32( dev, DMABSTRACTAUTO, 1 ); // Enable Autoexec.
		}
		iss->lastwriteflags = is_flash;


		iss->statetag = STTAG( "WRSQ" );
		iss->currentstateval = address_to_write;

		if( is_flash )
			ret |= WaitForDoneOp( dev );
	}
	else
	{
		if( address_to_write != iss->currentstateval )
		{
			MCFWriteReg32( dev, DMABSTRACTAUTO, 0 ); // Disable Autoexec.
			MCFWriteReg32( dev, DMDATA1R, address_to_write );
			MCFWriteReg32( dev, DMABSTRACTAUTO, 1 ); // Enable Autoexec.
		}
		MCFWriteReg32( dev, DMDATA0R, data );
		if( is_flash )
		{
			// XXX TODO: This likely can be a very short delay.
			// XXX POSSIBLE OPTIMIZATION REINVESTIGATE.
			ret |= WaitForDoneOp( dev );
		}
		else
		{
			ret |= WaitForDoneOp( dev );
		}
	}


	iss->currentstateval += 4;

	return 0;
}

static int UnlockFlash( struct SWIOState * iss )
{
	struct SWIOState * dev = iss;

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
		printf( "Whole-chip erase\n" );
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

	int blob_size = 64;
	uint32_t wp = address_to_write;
	uint32_t ew = wp + blob_size;
	int group = -1;
	int is_flash = 0;
	int rw = 0;

	if( (address_to_write & 0xff000000) == 0x08000000 || (address_to_write & 0xff000000) == 0x00000000  || (address_to_write & 0x1FFFF800) == 0x1FFFF000  ) 
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
		rw = EraseFlash( dev, address_to_write, blob_size, 0 );
		if( rw ) return rw;
		// 16.4.6 Main memory fast programming, Step 5
		//if( WaitForFlash( dev ) ) return -11;
		//WriteWord( dev, 0x40022010, FLASH_CTLR_BUF_RST );
		//if( WaitForFlash( dev ) ) return -11;

	}

	/* General Note:
		Most flash operations take about 3ms to complete :(
	*/

	while( wp < ew )
	{
		if( is_flash )
		{
			group = (wp & 0xffffffc0);
			WriteWord( dev, 0x40022010, CR_PAGE_PG ); // THIS IS REQUIRED, (intptr_t)&FLASH->CTLR = 0x40022010   (PG Performs quick page programming operations.)
			WriteWord( dev, 0x40022010, CR_BUF_RST | CR_PAGE_PG );  // (intptr_t)&FLASH->CTLR = 0x40022010

			int j;
			for( j = 0; j < 16; j++ )
			{
				int index = (wp-address_to_write);
				uint32_t data = 0xffffffff;
				if( index + 3 < blob_size )
					data = ((uint32_t*)blob)[index/4];
				else if( (int32_t)(blob_size - index) > 0 )
				{
					memcpy( &data, &blob[index], blob_size - index );
				}
				WriteWord( dev, wp, data );
				//if( (rw = WaitForFlash( dev ) ) ) return rw;
				wp += 4;
			}
			WriteWord( dev, 0x40022014, group );
			WriteWord( dev, 0x40022010, CR_PAGE_PG|CR_STRT_Set );  // R32_FLASH_CTLR
			if( (rw = WaitForFlash( dev ) ) ) return rw;
		}
		else
		{
			int index = (wp-address_to_write);
			uint32_t data = 0xffffffff;
			if( index + 3 < blob_size )
				data = ((uint32_t*)blob)[index/4];
			else if( (int32_t)(blob_size - index) > 0 )
				memcpy( &data, &blob[index], blob_size - index );
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
	r = MCFReadReg32( dev, DMDATA0R, &rr );
	if( r < 0 ) return r;

	if( maxlen < 8 ) return -9;

	// DMDATA1R:
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
				r = MCFReadReg32( dev, DMDATA1R, &r2 );
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
			MCFWriteReg32( dev, DMDATA1R, leavevalB );
		}
		MCFWriteReg32( dev, DMDATA0R, leavevalA ); // Write that we acknowledge the data.
		return ret;
	}
	else
	{
		return 0;
	}
}

#endif // _CH32V003_SWIO_H


