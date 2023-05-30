#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "hidapi.c"

int main()
{
	hid_device * hd = hid_open( 0xabcd, 0x8266, 0); // third parameter is "serial"

	printf( "HD: %p\n", hd );
	if( !hd ) return -4;

	uint8_t buffer0[32] = { 0 };
	uint8_t buffer1[32] = { 0 };
	int r;

	int i;
	int j;
	for( j = 0; ; j++ )
	{
		buffer0[0] = 0xaa;
		int i;
		for( i = 1; i < sizeof( buffer0 ); i ++ )
			buffer0[i] = rand(); 
		r = hid_send_feature_report( hd, buffer0, sizeof(buffer0)+1 );
	/*	printf( "%d: ", r );
		for( i = 0; i < r; i++ )
			printf( "%02x ", buffer[i] );
		printf( "\n" );*/
		printf( "<" );

		buffer1[0] = 0xaa;
		r = hid_get_feature_report( hd, buffer1, sizeof(buffer1) );
		printf( ">" ); fflush( stdout);
		if( r != sizeof( buffer1 ) ) { printf( "Got %d\n", r ); break; }
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

