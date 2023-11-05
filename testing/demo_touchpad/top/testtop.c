#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// We borrow the combined hidapi.c from minichlink.
//
// This is for total perf testing.

#include "hidapi.c"

int main()
{
	hid_device * hd = hid_open( 0x1209, 0xc003, L"CUSTOMDEVICE000"); // third parameter is "serial"
	if( !hd )
	{
		fprintf( stderr, "Error: Failed to open device.\n" );
		return -4;
	}

	// Size of buffers must match the report descriptor size in the special_hid_desc
	//  NOTE: You are permitted to have multiple entries.
	uint8_t buffer1[33] = { 0 }; // NOTE: This must be ONE MORE THAN what is in the hid descriptor.
	int r;
	int i;
	int j;
	int retries = 0;
	for( j = 0; ; j++ )
	{
		buffer1[0] = 0xaa; // First byte must be ID.

		r = hid_get_feature_report( hd, buffer1, sizeof(buffer1) );
		int i;
		int16_t * list = (uint16_t*)(buffer1+2);
		for( i = 0; i < 8; i++ )
		{
			printf( "%6d", list[i] );
		}
		printf( "\n" );
	}

	hid_close( hd );
}

