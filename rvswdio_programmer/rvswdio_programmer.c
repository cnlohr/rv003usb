#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

// For RVBB_REMAP
//        PC1+PC2 = SWIO (Plus 5.1k pull-up to PD2)
// Otherwise (Ideal for 8-pin SOIC, because PC4+PA1 are bound)
//        PC4+PA1 = SWIO (Plus 5.1k pull-up to PD2)
//
// By Default:
//        PD2 = Power control
//
// If programming V2xx, V3xx, X0xx, DEFAULT:
//        PC5 = SWCLK
//
// If you are using the MCO feature
//        PC4 = MCO (optional)
//
#define RVBB_REMAP 1 // To put SWD on PC1/PC2

#define IRAM_ATTR
#define MAX_IN_TIMEOUT 1000
#define R_GLITCH_HIGH
//#define RUNNING_ON_ESP32S2
#define BB_PRINTF_DEBUG printf
#include "bitbang_rvswdio.h"

#define PROGRAMMER_PROTOCOL_NUMBER 4

#define PIN_SWCLK   PC5     // Remappable.
#define PIN_TARGETPOWER PD2 // Remappable

#include "bitbang_rvswdio_ch32v003.h"

struct SWIOState state;

// Allow reading and writing to the scratchpad via HID control messages.
uint8_t scratch[72];
uint8_t retbuff[72];
volatile uint8_t scratch_run = 0;
volatile uint8_t scratch_return = 0;
uint8_t programmer_mode = 0;

static void SetupProgrammer(void)
{
	state.t1coeff = 0;
	programmer_mode = 0;
}

static void SwitchMode( uint8_t ** liptr, uint8_t ** lretbuffptr )
{
	programmer_mode = *((*liptr)++);
	// Unknown Programmer
	*((*lretbuffptr)++) = PROGRAMMER_PROTOCOL_NUMBER;
	*((*lretbuffptr)++) = programmer_mode;
	printf( "PM %d\n", programmer_mode );
	if( programmer_mode == 0 )
	{
		SetupProgrammer();
	}
}


static void HandleCommandBuffer( uint8_t * buffer )
{
	// Is send.
	// buffer[0] is the request ID.
	uint8_t * iptr = &buffer[1];
	uint8_t * retbuffptr = retbuff+1;
	#define reqlen (sizeof(scratch)-1)

	while( iptr - buffer < reqlen )	
	{
		uint8_t cmd = *(iptr++);
		int remain = reqlen - (iptr - buffer);
		// Make sure there is plenty of space.
		if( (sizeof(retbuff)-(retbuffptr - retbuff)) < 6 ) break;

		if( programmer_mode == 0 )
		{
			if( cmd == 0xfe ) // We will never write to 0x7f.
			{
				cmd = *(iptr++);

				switch( cmd )
				{
				case 0xfd:
					SwitchMode( &iptr, &retbuffptr );
					break;
				case 0x0e:
				case 0x01:
				{
					//DoSongAndDanceToEnterPgmMode( &state ); was 1.  But really we just want to init.
					// if we expect this, we can use 0x0e to get status.
					funPinMode( PIN_TARGETPOWER, GPIO_CFGLR_OUT_10Mhz_PP );
					funDigitalWrite( PIN_TARGETPOWER, 1 );
					int r = InitializeSWDSWIO( &state );
					if( cmd == 0x0e )
						*(retbuffptr++) = r;
					break;
				}
				case 0x02: // Power-down 
					printf( "Power down\n" );
					funDigitalWrite( PIN_TARGETPOWER, 0 );
					break;
				case 0x03: // Power-up
					funPinMode( PIN_TARGETPOWER, GPIO_CFGLR_OUT_10Mhz_PP );
					funDigitalWrite( PIN_TARGETPOWER, 1 );
					break;
				case 0x04: // Delay( uint16_t us )
					Delay_Us(iptr[0] | (iptr[1]<<8) );
					iptr += 2;
					break;
				case 0x05: // Void High Level State
					ResetInternalProgrammingState( &state );
					break;
				case 0x06: // Wait-for-flash-op.
					*(retbuffptr++) = WaitForFlash( &state );
					break;
				case 0x07: // Wait-for-done-op.
					*(retbuffptr++) = WaitForDoneOp( &state );
					break;
				case 0x08: // Write Data32.
				{
					if( remain >= 9 )
					{
						int r = WriteWord( &state, iptr[0] | (iptr[1]<<8) | (iptr[2]<<16) | (iptr[3]<<24),  iptr[4] | (iptr[5]<<8) | (iptr[6]<<16) | (iptr[7]<<24) );
						iptr += 8;
						*(retbuffptr++) = r;
					}
					break;
				}
				case 0x09: // Read Data32.
				{
					if( remain >= 4 )
					{
						int r = ReadWord( &state, iptr[0] | (iptr[1]<<8) | (iptr[2]<<16) | (iptr[3]<<24), (uint32_t*)&retbuffptr[1] );
						iptr += 4;
						retbuffptr[0] = r;
						if( r < 0 )
							memset( &retbuffptr[1], 0, 4 );
						retbuffptr += 5;
					}
					break;
				}
				case 0x0a:
					ResetInternalProgrammingState( &state );
					break;
				case 0x0b:
					if( remain >= 68 )
					{
						int r = Write64Block( &state, iptr[0] | (iptr[1]<<8) | (iptr[2]<<16) | (iptr[3]<<24), (uint8_t*)&iptr[4] );
						iptr += 68;
						*(retbuffptr++) = r;
					}
					break;
				case 0x0c:
					if( remain >= 8 )
					{
						// On the ESP32-S2, this will output clock on P2.
						int use_apll = *(iptr++);  // try 1
						int sdm0 = *(iptr++);      // try 0
						iptr += 6; // reserved + SDM values.

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
					break;
				case 0x0d:  // Do a terminal log through.
				{
					int tries = 100;
					if( remain >= 8 )
					{
						int r;
						uint32_t leavevalA = iptr[0] | (iptr[1]<<8) | (iptr[2]<<16) | (iptr[3]<<24);
						iptr += 4;
						uint32_t leavevalB = iptr[0] | (iptr[1]<<8) | (iptr[2]<<16) | (iptr[3]<<24);
						iptr += 4;
						uint8_t * origretbuf = (retbuffptr++);
						int canrx = (sizeof(retbuff)-(retbuffptr - retbuff)) - 8;
						while( canrx > 8 )
						{
							r = PollTerminal( &state, retbuffptr, canrx, leavevalA, leavevalB );
							origretbuf[0] = r;
							if( r >= 0 )
							{
								retbuffptr += r;
								if( tries-- <= 0 ) break; // ran out of time?
							}
							else
							{
								break;
							}
							canrx = (sizeof(retbuff)-(retbuffptr - retbuff)) -8;
							// Otherwise all is well.  If we aren't signaling try to poll for more data.
							if( leavevalA != 0 || leavevalB != 0 ) break;
						}
					}
					break;
				}
				case 0x0f: // Override chip type, etc.
					if( remain >= 8 )
					{
						state.target_chip_type = *(iptr++);
						state.sectorsize = iptr[0] | (iptr[1]<<8);
						iptr += 2;
						// Rest is reserved.
						iptr += 5;
						*(retbuffptr++) = 0; // Reply is always 0.
					}
					break;
				// Done
				}
			} else if( cmd == 0xff )
			{
				break;
			}
			else
			{
				// Otherwise it's a regular command.
				// 7-bit-cmd .. 1-bit read(0) or write(1) 
				// if command lines up to a normal QingKeV2 debug command, treat it as that command.

				if( cmd & 1 )
				{
					if( remain >= 4 )
					{
						MCFWriteReg32( &state, cmd>>1, iptr[0] | (iptr[1]<<8) | (iptr[2]<<16) | (iptr[3]<<24) );
						iptr += 4;
					}
				}
				else
				{
					if( remain >= 1 && (sizeof(retbuff)-(retbuffptr - retbuff)) >= 4 )
					{
						int r = MCFReadReg32( &state, cmd>>1, (uint32_t*)&retbuffptr[1] );
						retbuffptr[0] = r;
						if( r < 0 )
							memset( &retbuffptr[1], 0, 4 );
						retbuffptr += 5;
					}
				}
			}
		}
		else if( programmer_mode == 1 )
		{
			if( cmd == 0xff ) break;
#if 0
			switch( cmd )
			{
			case 0xfe:
			{
				cmd = *(iptr++);
				if( cmd == 0xfd )
				{
					SwitchMode( &iptr, &retbuffptr );
				}
				break;
			}
			case 0x90:
			{
				REG_WRITE( IO_MUX_GPIO6_REG, 1<<FUN_IE_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA drive.  Optional: 10k pull-up resistor.
				REG_WRITE( IO_MUX_GPIO7_REG, 1<<FUN_IE_S | 3<<FUN_DRV_S );  //VCC for part 40mA drive.
				rtc_cpu_freq_config_t m;
				rtc_clk_cpu_freq_get_config( &m );

				if( (sizeof(retbuff)-(retbuffptr - retbuff)) >= 18 && remain >= 2 )
				{
					int baudrate = *(iptr++);
					baudrate |= (*(iptr++))<<8;

					updi_clocks_per_bit = UPDIComputeClocksPerBit( m.freq_mhz, baudrate*32 );


					UPDIPowerOn( pinmask, pinmaskpower );
					uint8_t sib[17] = { 0 };
					int r = UPDISetup( pinmask, m.freq_mhz, updi_clocks_per_bit, sib );
					printf( "UPDISetup() = %d -> %s\n", r, sib );

					retbuffptr[0] = r;
					memcpy( retbuffptr + 1, sib, 17 );
					retbuffptr += 18;
				}
				break;
			}
			case 0x91:
			{
				REG_WRITE( IO_MUX_GPIO6_REG, 1<<FUN_IE_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA drive.  Optional: 10k pull-up resistor.
				REG_WRITE( IO_MUX_GPIO7_REG, 1<<FUN_IE_S | 3<<FUN_DRV_S );  //VCC for part 40mA drive.
				UPDIPowerOn( pinmask, pinmaskpower );
				break;
			}
			case 0x92:
			{
				REG_WRITE( IO_MUX_GPIO6_REG, 1<<FUN_IE_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA drive.  Optional: 10k pull-up resistor.
				REG_WRITE( IO_MUX_GPIO7_REG, 1<<FUN_IE_S | 3<<FUN_DRV_S );  //VCC for part 40mA drive.

				UPDIPowerOff( pinmask, pinmaskpower );
				break;
			}
			case 0x93: // Flash 64-byte block.
			{
				REG_WRITE( IO_MUX_GPIO6_REG, 1<<FUN_IE_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA drive.  Optional: 10k pull-up resistor.
				REG_WRITE( IO_MUX_GPIO7_REG, 1<<FUN_IE_S | 3<<FUN_DRV_S );  //VCC for part 40mA drive.

				if( remain >= 2+64 )
				{
					int addytowrite = *(iptr++);
					addytowrite |= (*(iptr++))<<8;
					int r;
					r = UPDIFlash( pinmask, updi_clocks_per_bit, addytowrite, iptr, 64, 0);
					printf( "Flash Response: %d\n", r );
					iptr += 64;

					*(retbuffptr++) = r;
				}
				break;
			}
			case 0x94:
			{
				REG_WRITE( IO_MUX_GPIO6_REG, 1<<FUN_IE_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA drive.  Optional: 10k pull-up resistor.
				REG_WRITE( IO_MUX_GPIO7_REG, 1<<FUN_IE_S | 3<<FUN_DRV_S );  //VCC for part 40mA drive.

				if( remain >= 3 )
				{
					int addytorx = *(iptr++);
					addytorx |= (*(iptr++))<<8;
					int bytestorx = *(iptr++);

					if( (sizeof(retbuff)-(retbuffptr - retbuff)) >= bytestorx + 1 )
					{
						retbuffptr[0] = UPDIReadMemoryArea( pinmask, updi_clocks_per_bit, addytorx, (uint8_t*)&retbuffptr[1], bytestorx );
						retbuffptr += bytestorx + 1;
					}
				}
				break;
			}
			case 0x95:
			{
				REG_WRITE( IO_MUX_GPIO6_REG, 1<<FUN_IE_S | 1<<FUN_DRV_S );  //Additional pull-up, 10mA drive.  Optional: 10k pull-up resistor.
				REG_WRITE( IO_MUX_GPIO7_REG, 1<<FUN_IE_S | 3<<FUN_DRV_S );  //VCC for part 40mA drive.

				*(retbuffptr++) = UPDIErase( pinmask, updi_clocks_per_bit );
				break;
			}
			case 0x96:
			{
				*(retbuffptr++) = UPDIReset( pinmask, updi_clocks_per_bit );

				break;
			}
			default:
#endif
				//UPDI Not implemented right now.
			{
				*(retbuffptr++) = 0xfe;
				*(retbuffptr++) = cmd;
				break;
			}
		}
		else
		{
			// Unknown Programmer
			*(retbuffptr++) = 0;
			*(retbuffptr++) = programmer_mode;

			break;
		}
	}
	retbuff[0] = (retbuffptr - retbuff) - 1;
}

int main()
{
	SystemInit();
	Delay_Ms(1); // Ensures USB re-enumeration after bootloader or reset; Spec demand >2.5µs ( TDDIS )
	usb_setup();

	funGpioInitAll();

	ConfigureIOForRVSWIO();

	Delay_Ms(10);

#if 0
	// This tests the system to see if we can bring it up.
	funDigitalWrite( PD2, 1 );
	Delay_Ms(1);

	int k;
	for( k = 0; k < 10; k++ )
	{
		printf( "Sending\n" );
		MCFWriteReg32( &state, DMSHDWCFGR, 0x5aa50000 | (1<<10) ); // Shadow Config Reg
		MCFWriteReg32( &state, DMCFGR, 0x5aa50000 | (1<<10) );     // CFGR (1<<10 == Allow output from slave)
		MCFWriteReg32( &state, DMSHDWCFGR, 0x5aa50000 | (1<<10) ); // Shadow Config Reg
		MCFWriteReg32( &state, DMCFGR, 0x5aa50000 | (1<<10) );     // CFGR (1<<10 == Allow output from slave)
		MCFWriteReg32( &state, DMCONTROL, 0x80000001 );            // Make the debug module work properly.
		MCFWriteReg32( &state, DMCONTROL, 0x80000001 );            // Re-initiate a halt request.

		uint32_t reg;
		int ret = MCFReadReg32( &state, DMSTATUS, &reg );

		printf( "%08x %d\n", (int)reg, ret );
	}
#endif

	while(1)
	{
		//printf( "%lu %lu %lu %08lx\n", rv003usb_internal_data.delta_se0_cyccount, rv003usb_internal_data.last_se0_cyccount, rv003usb_internal_data.se0_windup, RCC->CTLR );
#if RV003USB_EVENT_DEBUGGING
		uint32_t * ue;
		while( ( ue = GetUEvent() ) )
		{
			printf( "%lu %lx %lx %lx\n", ue[0], ue[1], ue[2], ue[3] );
		}
#endif
		if( scratch_run )
		{
			scratch_return = 0;
			HandleCommandBuffer( scratch );
			scratch_run = 0;
			scratch_return = 1;
		}
	}
}


void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	// Make sure we only deal with control messages.  Like get/set feature reports.
	if( endp )
	{
		usb_send_empty( sendtok );
	}
	else
	{
		LogUEvent( 5, endp, sendtok, scratch_run );
		if( scratch_run ) 
		{
			// Send NACK (can't accept any more data right now)
			usb_send_data( 0, 0, 2, 0x5A );
		}
		else
		{
			if( !e->max_len )
			{
				usb_send_empty( sendtok );
			}
			else
			{
				int len = e->max_len;
				int slen = len;
				if( slen > 8 ) slen = 8;
				usb_send_data( (uint8_t*)e->opaque, slen, 0, sendtok );
				e->opaque += 8;
				len -= 8;
				if( len < 0 ) len = 0;
				e->max_len = len;
				LogUEvent( 6, e->max_len, slen, len );
			}
		}
	}
}


void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
	if( scratch_run ) 
	{
		// Send NACK (can't accept any more data right now)
		usb_send_data( 0, 0, 2, 0x5A );
		return;
	}

	usb_send_data( 0, 0, 2, 0xD2 ); // Send ACK
	int offset = e->count<<3;
	int torx = e->max_len - offset;
	if( torx > len ) torx = len;
	if( torx > 0 )
	{
		memcpy( scratch + offset, data, torx );
		e->count++;
		if( ( e->count << 3 ) >= e->max_len )
		{
			scratch_run = e->max_len;
		}
	}
}

void usb_handle_hid_get_report_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
	if( reqLen > sizeof( scratch ) ) reqLen = sizeof( scratch );

	// You can check the lValueLSBIndexMSB word to decide what you want to do here
	// But, whatever you point this at will be returned back to the host PC where
	// it calls hid_get_feature_report. 
	//
	// Please note, that on some systems, for this to work, your return length must
	// match the length defined in HID_REPORT_COUNT, in your HID report, in usb_config.h

	e->opaque = retbuff;
	e->max_len = reqLen;
	e->custom = 1;
	scratch_return = 0;
}

void usb_handle_hid_set_report_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
	// Here is where you get an alert when the host PC calls hid_send_feature_report.
	//
	// You can handle the appropriate message here.  Please note that in this
	// example, the data is chunked into groups-of-8-bytes.
	//
	// Note that you may need to make this match HID_REPORT_COUNT, in your HID
	// report, in usb_config.h

	e->opaque = scratch;
	e->custom = 0;
	if( scratch_run ) reqLen = 0;
	if( reqLen > sizeof( scratch ) ) reqLen = sizeof( scratch );
	e->max_len = reqLen;
	LogUEvent( 2, reqLen, lValueLSBIndexMSB, 0 );
}



