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
//#define CRC16START 0x0000
//#define CRC16GOOD  0xb771
#define CRC16START 0xf0eb
#define CRC16GOOD  0x0000
#define CRC16POLY  0xa001

// CRC5 is  x^5 + x^2 + x^0
//#define CRC5START  0x1f
//#define CRC5GOOD   0x06

//#define CRC5START  0x00
//#define CRC5GOOD   0x16

#define CRC5START  0x1e
#define CRC5GOOD   0x00
#define CRC5POLY   0x14  // Reverse polynomial representation. 0x05 is normal

// We do reversed polynomials so we can >>' down the numbers and work with 
// numbers closer to the lsb's.  Which works well with compressed ISAs.

// BIG NOTE:
//  You can actually perform all of these CRCs one-byte-at-a-time.
//  OR you can perform them byte-at-a-time.

int main()
{
	int i;
	if( 0 )
	{
		// Polynomial seek to find a 0 start polynomial.
		int sp = 0;
		for( sp = 0; sp < 32; sp++ )
		{
			// First, check CRC5 the naive way.
			uint32_t crc5 = sp;
			uint8_t bitstream[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0, 1, 0, 0, 0 };

			for( i = 0; i < sizeof( bitstream ); i++ )
			{
				uint32_t bv = bitstream[i];
				uint32_t do_xor = (bv ^ crc5) & 1;
			    crc5 >>= 1;
				if( do_xor )
				    crc5 ^= CRC5POLY;
			}
			if( crc5 == 0 ) printf( "Found good start poly: %02x\n", sp );
		}
	}
	if( 1 )
	{
		int sp = 0;
		for( sp = 0; sp < 65536; sp++ )
		{
			// CRC16 end-poly-seek
			uint32_t crc16 = sp;
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

				// This is like the code we have in the USB stack.
				bv = bv?0:1;
				uint32_t polyxor = (((uint32_t)((bv ^ crc16))) & 1)-1;
				polyxor &= CRC16POLY;
			    crc16 >>= 1;
				crc16 ^= polyxor;
			}
			if( crc16 == 0 ) printf( "Found seeking poly: %04x\n", sp );
		}
	}

	{
		// First, check CRC5 the naive way.
		uint32_t crc5 = CRC5START;
		uint8_t bitstream[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0, 1, 0, 0, 0 };

		for( i = 0; i < sizeof( bitstream ); i++ )
		{
			uint32_t bv = bitstream[i];
			uint32_t do_xor = (bv ^ crc5) & 1;
	        crc5 >>= 1;
		    if( do_xor )
		        crc5 ^= CRC5POLY;
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

	{
		// Another CRC5 test.
		uint32_t crc5 = CRC5START;
		uint8_t bitstream[] = { 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1,    1, 1, 1, 0, 1 };
		for( i = 0; i < sizeof( bitstream ); i++ )
		{
			uint32_t bv = bitstream[i];

			uint32_t polyxor = -(((bv ^ crc5) & 1));
			polyxor &= CRC5POLY;
	        crc5 >>= 1;
			crc5 ^= polyxor;
		}
		printf( "%02x ?= %02x\n", crc5, CRC5GOOD );
	}


	{
		// An AWFUL CRC5 test.
		uint32_t crc5 = CRC5START;
		uint8_t bitstream[] = { 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1,    1, 1, 1, 0, 1 };
		for( i = 0; i < sizeof( bitstream ); i++ )
		{
			uint32_t bv = bitstream[i];
			bv -= 1;
			// This is like the code we have in the USB stack.

			uint32_t polyxor = (((bv ^ crc5) & 1))-1;
			polyxor &= CRC5POLY;
	        crc5 >>= 1;
			crc5 ^= polyxor;
		}
		printf( "Awful 5: %02x ?= %02x\n", crc5, CRC5GOOD );
	}



	// Regular CRC-16
	{
		// Another CRC16 test.
		uint32_t crc16 = CRC16START;
		uint8_t bitstream[] = { 
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
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
		printf( "Generic CRC16 Empty: %04x ?= %04x\n", crc16, CRC16GOOD );
	}
	{
		uint32_t crc16 = 0xffff;
		uint8_t bitstream[] = { 
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
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
		printf( "DATA1 w/ 0xAA+C0C0 CRC16: %04x ?= %04x\n", crc16, CRC16GOOD );

		int crccheck;
		int j;
		for( j = 0; j < 65536; j++ )
		{
			uint32_t crc16 = CRC16START;
			for( i = 0; i < 16; i++ )
			{
				uint32_t bv = (j>>i)&1;
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
			if( crc16 == 0 ) printf( "If CRC was %04x, it would pass.\n", j );
		}

	}



	// Cursed, test CRC-16
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
			bv -= 1;
			// This is like the code we have in the USB stack.

			uint32_t polyxor = (((uint32_t)((bv ^ crc16))) & 1)-1;
			polyxor &= CRC16POLY;
	        crc16 >>= 1;
			crc16 ^= polyxor;
		}
		printf( "Awful: %04x ?= %04x\n", crc16, CRC16GOOD );
	}



	// Another cursed, test CRC-16
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

			// This is like the code we have in the USB stack.
			bv = bv?0:1;
			uint32_t polyxor = (((uint32_t)((bv ^ crc16))) & 1)-1;
			polyxor &= CRC16POLY;
	        crc16 >>= 1;
			crc16 ^= polyxor;
		}
		printf( "Awful: %04x ?= %04x\n", crc16, CRC16GOOD );
	}

	// Tricky junk to un-compute? the beginning of the CRC16s for DATA0/DATA1.
	
	// Another cursed, test CRC-16
	{
		uint32_t startpoly = 0;
		for( startpoly = 0; startpoly != 0x10000; startpoly++ )
		{
			// Another CRC16 test.
			uint32_t crc16 = startpoly;
			uint8_t bitstream[] = { 
				0, 0, 0, 0, 0, 0, 0, 1,
				1, 1, 0, 1, 0, 0, 1, 0,
			};
			for( i = 0; i < sizeof( bitstream ); i++ )
			{
				uint32_t bv = bitstream[i];

				// This is like the code we have in the USB stack.
				bv = bv?0:1;
				uint32_t polyxor = (((uint32_t)((bv ^ crc16))) & 1)-1;
				polyxor &= CRC16POLY;
			    crc16 >>= 1;
				crc16 ^= polyxor;
			}
			if( crc16 == 0xffff ) break;
		}
		printf( "Preload Poly for DATA1: %04x\n", startpoly );

		for( startpoly = 0; startpoly != 0x10000; startpoly++ )
		{
			// Another CRC16 test.
			uint32_t crc16 = startpoly;
			uint8_t bitstream[] = { 
				0, 0, 0, 0, 0, 0, 0, 1,
				1, 1, 0, 0, 0, 0, 1, 1,
			};
			for( i = 0; i < sizeof( bitstream ); i++ )
			{
				uint32_t bv = bitstream[i];

				// This is like the code we have in the USB stack.
				bv = bv?0:1;
				uint32_t polyxor = (((uint32_t)((bv ^ crc16))) & 1)-1;
				polyxor &= CRC16POLY;
			    crc16 >>= 1;
				crc16 ^= polyxor;
			}
			if( crc16 == 0xffff ) break;
		}
		printf( "Preload Poly for DATA0: %04x\n", startpoly );



		for( startpoly = 0; startpoly != 0x10000; startpoly++ )
		{
			// Another CRC16 test.
			uint32_t crc16 = startpoly;
			uint8_t bitstream[] = { 
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			};
			for( i = 0; i < sizeof( bitstream ); i++ )
			{
				uint32_t bv = bitstream[i];

				// This is like the code we have in the USB stack.
				bv = bv?0:1;
				uint32_t polyxor = (((uint32_t)((bv ^ crc16))) & 1)-1;
				polyxor &= CRC16POLY;
			    crc16 >>= 1;
				crc16 ^= polyxor;
			}
			if( crc16 == 0x0000 ) break;
		}
		printf( "Preload Poly for NO DATA: %04x\n", startpoly );
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
		printf( "CRC16: %04x ?= %04x\n", crc16, CRC16GOOD );
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
	}

}

