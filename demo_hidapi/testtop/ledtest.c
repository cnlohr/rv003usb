#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// We borrow the combined hidapi.c from minichlink.
//
// This is for total perf testing.

#include "hidapi.c"

uint32_t EHSVtoHEX( uint8_t hue, uint8_t sat, uint8_t val );

const uint8_t gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

int main()
{
	hid_device * hd = hid_open( 0x1209, 0xd003, L"CUSTOMDEVICE000"); // third parameter is "serial"
	if( !hd )
	{
		fprintf( stderr, "Error: Failed to open device.\n" );
		return -4;
	}

	// Size of buffers must match the report descriptor size in the special_hid_desc
	//  NOTE: You are permitted to have multiple entries.
	uint8_t buffer0[255] = { 0 }; // NOTE: This must be ONE MORE THAN what is in the hid descriptor.
	uint8_t buffer1[255] = { 0 };
	int retries = 0;
	int j, i, r;
	for( j = 0; ; j++ )
	{
		buffer0[0] = 0xaa; // First byte must match the ID.
		buffer0[1] = 0xa4;
		// But we can fill in random for the rest.
		for( i = 4; i < sizeof( buffer0 )-2; i+=3 )
		{
			uint32_t rgb = EHSVtoHEX( i*24+j, 255, 255 );
			buffer0[i+0] = gamma8[(uint8_t)rgb];
			buffer0[i+1] = gamma8[(uint8_t)(rgb>>8)];
			buffer0[i+2] = gamma8[(uint8_t)(rgb>>16)];
		}

		retrysend:
		r = hid_send_feature_report( hd, buffer0, sizeof(buffer0) );
		if( r != sizeof(buffer0) )
		{
			fprintf( stderr, "Warning: HID Send fault (%d) Retrying\n", r );
			retries++;
			if( retries > 10 ) break;
			goto retrysend;
		}


		retries = 0;
		printf( "<" ); // Print this out meaning we sent the data.

		memset( buffer1, 0xff, sizeof( buffer1 ) );
		buffer1[0] = 0xaa; // First byte must be ID.

		r = hid_get_feature_report( hd, buffer1, sizeof(buffer1) );

		printf( ">" ); fflush( stdout);

		if( r != sizeof( buffer1 ) && r != sizeof( buffer1 ) + 1) { printf( "Got %d\n", r ); break; }

		// Validate the scratches matched.
		if( memcmp( buffer0, buffer1, sizeof( buffer0 ) ) != 0 ) 
		{
			printf( "%d: ", r );
			for( i = 0; i < r; i++ )
				printf( "[%d] %02x>%02x %s", i, buffer0[i], buffer1[i], (buffer1[i] != buffer0[i])?"MISMATCH ":""  );
			printf( "\n" );
			break;
		}
	}

	hid_close( hd );
}



uint32_t EHSVtoHEX( uint8_t hue, uint8_t sat, uint8_t val )
{
	#define SIXTH1 43
	#define SIXTH2 85
	#define SIXTH3 128
	#define SIXTH4 171
	#define SIXTH5 213

	uint16_t or = 0, og = 0, ob = 0;

	hue -= SIXTH1; //Off by 60 degrees.

	//TODO: There are colors that overlap here, consider 
	//tweaking this to make the best use of the colorspace.

	if( hue < SIXTH1 ) //Ok: Yellow->Red.
	{
		or = 255;
		og = 255 - ((uint16_t)hue * 255) / (SIXTH1);
	}
	else if( hue < SIXTH2 ) //Ok: Red->Purple
	{
		or = 255;
		ob = (uint16_t)hue*255 / SIXTH1 - 255;
	}
	else if( hue < SIXTH3 )  //Ok: Purple->Blue
	{
		ob = 255;
		or = ((SIXTH3-hue) * 255) / (SIXTH1);
	}
	else if( hue < SIXTH4 ) //Ok: Blue->Cyan
	{
		ob = 255;
		og = (hue - SIXTH3)*255 / SIXTH1;
	}
	else if( hue < SIXTH5 ) //Ok: Cyan->Green.
	{
		og = 255;
		ob = ((SIXTH5-hue)*255) / SIXTH1;
	}
	else //Green->Yellow
	{
		og = 255;
		or = (hue - SIXTH5) * 255 / SIXTH1;
	}

	uint16_t rv = val;
	if( rv > 128 ) rv++;
	uint16_t rs = sat;
	if( rs > 128 ) rs++;

	//or, og, ob range from 0...255 now.
	//Need to apply saturation and value.

	or = (or * val)>>8;
	og = (og * val)>>8;
	ob = (ob * val)>>8;

	//OR..OB == 0..65025
	or = or * rs + 255 * (256-rs);
	og = og * rs + 255 * (256-rs);
	ob = ob * rs + 255 * (256-rs);
//printf( "__%d %d %d =-> %d\n", or, og, ob, rs );

	or >>= 8;
	og >>= 8;
	ob >>= 8;

	return or | (og<<8) | ((uint32_t)ob<<16);
}
