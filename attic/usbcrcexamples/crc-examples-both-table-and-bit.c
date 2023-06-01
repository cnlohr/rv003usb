// USB CRC-5 and USB CRC-16 demos BOTH naive and byte-at-a-time
// 2023 Charles Lohr
//
// This file is just math.  It and all portions thereof are public domain.
// If youare in a judisdiction where public domain is not recognized for
// mathematical algorithms, you may license this file under the MIT-x11
// NewBSD, or CC0 licenses.

#include <stdio.h>
#include <stdint.h>

// This file from the table generation code here:
// https://github.com/cnlohr/espusb/blob/master/table/table.c
// It took me some time to decypher this:
// https://www.usb.org/sites/default/files/crcdes.pdf


// See this table: https://en.wikipedia.org/wiki/Cyclic_redundancy_check
// CRC16 USB is x^16 + x^15 + x^2 + x^0
//#define CRC16START 0xffff // This the normal way to do it. We go backwards.
//#define CRC16GOOD  0xB001
#define CRC16START 0x0000
#define CRC16GOOD  0xb771
#define CRC16POLY  0xA001

// CRC5 is  x^5 + x^2 + x^0
//#define CRC5START  0x1f
//#define CRC5GOOD   0x06
#define CRC5START  0x00
#define CRC5GOOD   0x16
#define CRC5POLY   0x14  // Reverse polynomial representation. 0x05 is normal

// We do reversed polynomials so we can >>' down the numbers and work with 
// numbers closer to the lsb's.  Which works well with compressed ISAs.

// BIG NOTE:
//  You can actually perform all of these CRCs one-byte-at-a-time.
//  OR you can perform them byte-at-a-time.

int main()
{
	int i;

	{
		// First, check CRC5 the naive way.
		uint32_t crc5 = CRC5START;
		uint8_t bitstream[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0, 1, 0, 0, 0 };

		for( i = 0; i < sizeof( bitstream ); i++ )
		{
			uint32_t bv = bitstream[i];
		    if ( (bv ^ crc5) & 1 )
		    {
		        crc5 >>= 1;
		        crc5 ^= CRC5POLY;
		    }
		    else
			{
		        crc5 >>= 1;
			}
		}
		printf( "%02x ?= %02x\n", crc5, CRC5GOOD );
	}

	{
		// Another CRC5 test.
		uint32_t crc5 = CRC5START;
		uint8_t bitstream[] = { 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1,    1, 1, 1, 0, 1 };
		for( i = 0; i < sizeof( bitstream ); i++ )
		{
			uint32_t bv = bitstream[i];
		    if ( (bv ^ crc5) & 1 )
		    {
		        crc5 >>= 1;
		        crc5 ^= CRC5POLY;
		    }
		    else
			{
		        crc5 >>= 1;
			}
		}
		printf( "%02x ?= %02x\n", crc5, CRC5GOOD );
	}

	// Now, test CRC-16
	{
		// Another CRC16 test.
		uint32_t crc16 = CRC16START;
		uint8_t bitstream[] = { 
			0, 0, 0, 0, 0, 0, 0, 1,
			0, 1, 1, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 1, 1, 1, 0, 1, 1,
			0, 0, 1, 0, 1, 0, 0, 1,
		};
		for( i = 0; i < sizeof( bitstream ); i++ )
		{
			uint32_t bv = bitstream[i];
		    if ( (bv ^ crc16) & 1 )
		    {
		        crc16 >>= 1;
		        crc16 ^= CRC16POLY;
		    }
		    else
			{
		        crc16 >>= 1;
			}
		}
		printf( "%04x ?= %04x\n", crc16, CRC16GOOD );
	}


	// CRC 5 table technique - typically 8x or more faster than 1-bit-at-a-time
	{
		uint8_t crc5table[256];
		for( i = 0; i < 256; i++ )
		{
			// Create table from all possible inputs
			uint32_t crc5 = 0;
			int j;

			for( j = 0; j < 8; j++ )
			{
				uint32_t bv = (i>>j)&1;
				if ( (bv ^ crc5) & 1 )
				{
				    crc5 >>= 1;
				    crc5 ^= CRC5POLY;
				}
				else
				{
				    crc5 >>= 1;
				}
			}
			crc5table[i] = crc5;
		}

		//Test
		{
			// { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0, 1, 0, 0, 0 };
			// Translation of the above, as LSB first.
			uint8_t bytestream[] = { 0x00, 0x10 };
			uint8_t crc5 = CRC5START;
			int j;
			for( j = 0; j < 2; j++ )
			{
				crc5 = crc5table[bytestream[j]^crc5];
			}
			printf( "%02x\n", crc5 );
		}

		{
			// { 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1,    1, 1, 1, 0, 1 };
			// Translation of the above, as LSB first.
			uint8_t bytestream[] = { 0x47, 0xbd };
			uint8_t crc5 = CRC5START;
			int j;
			for( j = 0; j < 2; j++ )
			{
				crc5 = crc5table[bytestream[j]^crc5];
			}
			printf( "%02x\n", crc5 );
		}

		printf( "uint8_t crc5table[] = {" );
		for( i = 0; i < 256; i++ )
		{
			if( ( i & 0xf ) == 0 ) printf( "\n\t" );
			printf( "0x%02x, ", crc5table[i] );
		}
		printf( "\n};\n" );
	}

	// CRC 16 table technique - typically 8x or more faster than 1-bit-at-a-time
	{
		uint16_t crc16table[256];
		for( i = 0; i < 256; i++ )
		{
			uint32_t crc16 = 0;
			int j;

			for( j = 0; j < 8; j++ )
			{
				uint32_t bv = (i>>j)&1;
				if ( (bv ^ crc16) & 1 )
				{
				    crc16 >>= 1;
				    crc16 ^= CRC16POLY;
				}
				else
				{
				    crc16 >>= 1;
				}
			}
			crc16table[i] = crc16;
		}

		//Test
		{
			uint8_t bitstream[] = { 
				0, 0, 0, 0, 0, 0, 0, 1,
				0, 1, 1, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				1, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 1, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				1, 0, 1, 1, 1, 0, 1, 1,
				0, 0, 1, 0, 1, 0, 0, 1,
			};
			// Translation of the above, as LSB first.
			uint8_t bytestream[] = { 0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x40, 0x00, 0xdd, 0x94 };
			uint32_t crc16 = CRC16START;
			int j;
			for( j = 0; j < sizeof( bytestream ); j++ )
			{
				crc16 = (crc16>>8) ^ crc16table[ (bytestream[j]^crc16) & 0xff ];
			}
			crc16 &= 0xffff;
			printf( "%04x ?= %04x\n", crc16, CRC16GOOD );
		}

		printf( "uint16_t crc16table[] = {" );
		for( i = 0; i < 256; i++ )
		{
			if( ( i & 0xf ) == 0 ) printf( "\n\t" );
			printf( "0x%04x, ", crc16table[i] );
		}
		printf( "\n};\n" );
	}}

