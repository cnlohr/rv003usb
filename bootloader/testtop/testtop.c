#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "hidapi.c"

int main()
{
	hid_device * hd = hid_open( 0xCDCD, 0x1563, 0); // third parameter is "serial"

	printf( "HD: %p\n", hd );
	if( !hd ) return -4;

	uint8_t buffer[64] = { 0 };
	int r;

	int i;
	for( i = 0; i < sizeof( buffer ); i++ )
		buffer[i] = i;
	int j;
	for( j = 0; ; j++ )
	{
		buffer[0] = 0xaa;
		buffer[1] = 0xff;
		buffer[2] = 0xd7;
		buffer[3] = 0x37;
		buffer[4] = 0xf8;
		r = hid_send_feature_report( hd, buffer, sizeof(buffer) );
	/*	printf( "%d: ", r );
		for( i = 0; i < r; i++ )
			printf( "%02x ", buffer[i] );
		printf( "\n" );*/
		printf( "<" );

		buffer[0] = 0xaa;
		r = hid_get_feature_report( hd, buffer, sizeof(buffer) );
/*		printf( "%d: ", r );
		for( i = 0; i < r; i++ )
			printf( "%02x ", buffer[i] );
		printf( "\n" );*/
		printf( ">" ); fflush( stdout);
		if( r != sizeof( buffer ) ) break;
	}

	hid_close( hd );
}

