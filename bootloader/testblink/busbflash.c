#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "hidapi.c"

int main()
{
	hid_device * hd = hid_open( 0x1209, 0xb003, 0); // third parameter is "serial"

	printf( "HD: %p\n", hd );
	if( !hd ) return -4;

	uint8_t buffer0[128] = { 0 };
	uint8_t buffer1[128+1] = { 0 };
	int r;

	// First, load the 
	FILE * f = fopen( "testblink.bin", "r" );
	if( !f )
	{
		fprintf( stderr, "Error: Can't open testblink.bin\n" );
		return -5;
	}

	r = fread( buffer0, 1, 128, f );
	printf( "Read %d bytes\n", r );
	if( r > 124 ) 
	{
		fprintf( stderr, "Image too big for scratchpad\n" );
		return -9;
	}

	buffer0[124] = 0xcd;
	buffer0[125] = 0xab;
	buffer0[126] = 0x34;
	buffer0[127] = 0x12;

	r = hid_send_feature_report( hd, buffer0, sizeof(buffer0) );
	printf( "Send feature report: %d\n", r );
	usleep( 20000 );

	buffer0[0] = 0xaa;
	r = hid_get_feature_report( hd, buffer1, sizeof(buffer1)+1 );
	printf( "Read: %d\n", r ); fflush( stdout);

	if( r != sizeof( buffer1 ) ) { printf( "Got %d\n", r ); }
	if( memcmp( buffer0, buffer1+1, sizeof( buffer0 ) - 4) != 0 ) 
	{
		int i;
		printf( "%d: ", r );
		for( i = 0; i < r; i++ )
			printf( "[%d] %02x>%02x %s", i, buffer0[i], buffer1[i+1], (buffer1[i+1] != buffer0[i])?"MISMATCH ":""  );
		printf( "\n" );
	}
	else
	{
		printf( "Readback Confirm\n" );
	}


	hid_close( hd );
}

