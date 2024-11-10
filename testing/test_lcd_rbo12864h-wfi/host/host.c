#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include "hidapi.c"
#include "os_generic.h"

#define CNFG_IMPLEMENTATION
#define CNFGOGL
#include "rawdraw_sf.h"

int ldx, ldy, ldd;
int brightness = 135;
int resistor_ratio = 100;
int backlight = 200;
int black_level = 200;

void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { ldd = 1<<bDown; ldx = x; ldy = y; }
void HandleMotion( int x, int y, int mask ) { ldd = mask; ldx = x; ldy = y; }
int HandleDestroy() { return 0; }

#define USB_VENDOR_ID  0xabcd
#define USB_PRODUCT_ID 0xc733

#define USB_TIMEOUT 1024

int framesThisSecond;
int abortLoop = 0;

static Display *display;
static XImage *image;
static XShmSegmentInfo shminfo;
static uint32_t * imagedata;
static Window capturewindow;

static void sighandler(int signum)
{

    XShmDetach(display, &shminfo);
    XDestroyImage(image);
    shmdt(shminfo.shmaddr);
    shmctl(shminfo.shmid, IPC_RMID, 0);
    XCloseDisplay(display);

	printf( "\nInterrupt signal received\n" );
	abortLoop = 1;
}

int xfertotal = 0;
int didstart = 0;


int Xerrhandler(Display * d, XErrorEvent * e)
{
	printf( "XShmGetImage error\n" );
    return 0;
}

void DrawBar( int level, int x, const char * name )
{
	CNFGColor( 0xffffffff );
	CNFGTackRectangle( 4+x, 6, 34+x, 264 );
	CNFGColor( 0x000000ff );
	CNFGTackRectangle( 5+x, 7, 33+x, 263 );
	CNFGColor( 0x0000ffff );
	CNFGTackRectangle( 5+x, 263-level, 33+x, 263 );
	CNFGColor( 0xffffffff );
	CNFGPenX = 6+x;
	CNFGPenY = 264;
	char cts[1024];
	sprintf( cts, "%s%d", name, level );
	CNFGDrawText( cts, 2 );
}


int frame = 0;
const int scale = 4;
const int screen_w = 128;
const int screen_h = 64;

int main(int argc, char **argv)
{
	//Pass Interrupt Signal to our handler
	signal(SIGINT, sighandler);


	XInitThreads();
	display = XOpenDisplay(NULL);
	//XSetErrorHandler(Xerrhandler);


	int major = 0, minor = 0;
	Bool pixmaps = 0;
	XShmQueryVersion(display, &major, &minor, &pixmaps);
	printf("SHM Version %d.%d, Pixmaps supported: %s\n",
		   major, minor,
		   pixmaps ? "yes" : "no");

	shminfo.shmid = shmget(IPC_PRIVATE, screen_w*4*scale*screen_h*scale, IPC_PRIVATE | IPC_EXCL | IPC_CREAT | 0777 );
	shminfo.shmaddr = shmat(shminfo.shmid, 0, 0);
	shminfo.readOnly = False;

	capturewindow = DefaultRootWindow(display);

	XWindowAttributes attribs;
	XGetWindowAttributes(display, capturewindow, &attribs);

	image = XShmCreateImage(display, 
		attribs.visual,
		attribs.depth,   // Determine correct depth from the visual. Omitted for brevity
		ZPixmap, NULL, &shminfo, screen_w*scale, screen_h*scale);

	didstart = 1;

	int att = XShmAttach( display, &shminfo );
	image->data = shminfo.shmaddr;
	imagedata = malloc( screen_w*4*scale*screen_h*scale );

	CNFGSetup( "Example App", 320, 290 );

	int is_reconnecting = 0;
reconnect:
	hid_device * hd = hid_open( 0xabcd, 0xc733, L"000"); // third parameter is "serial"
	if( !hd )
	{
		fprintf( stderr, "Error: Failed to open device.\n" );
		if( is_reconnecting )
		{
			usleep(300000);
			goto reconnect;
		}
		return -4;
	}
	is_reconnecting = 1;

	// Size of buffers must match the report descriptor size in the special_hid_desc
	//  NOTE: You are permitted to have multiple entries.
	uint8_t buffer0[1064] = { 0 }; // NOTE: This must be ONE MORE THAN what is in the hid descriptor.
	int r;
	int i;
	int j;
	int retries = 0;
	double dStart = OGGetAbsoluteTime();
	double dSecond = dStart;
	double dStartSend = 0.0;
	double dSendTotal = 0;
	double dRecvTotal = 0;

	static int nStoreFPS;
	static double dStoreSendTimeS;
	for( j = 0; ; j++ )
	{
		frame++;
		framesThisSecond++;
		CNFGHandleInput();
		CNFGClearFrame();

		DrawBar( brightness, 0, "CR:" );
		DrawBar( resistor_ratio, 60, "RR:" );
		DrawBar( backlight, 120, "BL:" );
		DrawBar( black_level, 180, "CL:" );


		if( ldd )
		{
			int brt = 265 - ldy;
			if( brt < 0 ) brt = 0;
			if( brt >= 255 ) brt = 255;

			if( ldx > 4 && ldx < 35 )
				brightness = brt;
			if( ldx > 60 && ldx < 94 )
				resistor_ratio = brt;
			if( ldx > 120 && ldx < 164 )
				backlight = brt;
			if( ldx > 180 && ldx < 214 )
				black_level = brt;
		}

		CNFGSwapBuffers();

		XShmGetImage(display, capturewindow, image, 200, 200, AllPlanes);
		memcpy( imagedata, (uint32_t*)shminfo.shmaddr, screen_w*4*scale*screen_h*scale );

		#define ST7567_COL_ADDR_H       0x10 // x pos (0..95) 4 MSB
		#define ST7567_COL_ADDR_L       0x00 // x pos (0..95) 4 LSB
		#define ST7567_PAGE_ADDR        0xB0 // y pos, 8.5 rows (0..8)
		#define ST7567_RMW              0xE0

		uint8_t * pb = (uint8_t*)&buffer0[0];
		memset( pb, 0, sizeof(buffer0) );

		*(pb++) = 0xab;
		*(pb++) = backlight;

		*(pb++) = 0x81;
		*(pb++) = brightness>>2;
		*(pb++) = 0x20 + (resistor_ratio>>5);
		*(pb++) = 0x00;

		int x, y;
		for( y = 0; y < screen_h; y+=8 )
		{
			*(pb++) = ST7567_PAGE_ADDR | (y>>3);
			*(pb++) = ST7567_COL_ADDR_H | 0;
			*(pb++) = ST7567_COL_ADDR_L | 1;
			*(pb++) = ST7567_RMW;
			for( x = 0; x < screen_w; x++ )
			{
				int row;
				int prow = 0;
				for( row = 0; row < 8; row++ )
				{
					const int flip = 1;
					int ry = (y+row);
					int rx = x;
					if( flip ) ry = screen_h - ry - 1;
					if( flip ) rx = screen_w - x - 1;
					uint32_t d = imagedata[screen_w*(ry)*scale*scale+rx*scale];
					int b = d&0xff;
					int g = (d>>8)&0xff;
					int r = (d>>16)&0xff;

					b = b * 252 / 256;
					g = g * 254 / 256;
					r = r * 252 / 256;

					int eo = (i+frame+row+y)&1;
					// Dithering.
					if( eo )
					{
						b += 1<<2;
						g += 1<<1;
						r += 1<<2;
						if( r > 255 ) r = 255;
						if( g > 255 ) g = 255;
						if( b > 255 ) b = 255;
					}
					int px = (r+g+b) < black_level*3;
					prow |= px<<row;
					//uint16_t v = (b>>3) | ((g>>2) << 5) | ((r>>3) << 11 );
				}
				*(pb++) = prow;
			}
		}

		// Leftover
		*(pb++) = 0xaa;
		*(pb++) = 0xaa;
		
		dStartSend = OGGetAbsoluteTime();
		r = hid_send_feature_report( hd, buffer0, pb-buffer0 );
		dSendTotal += OGGetAbsoluteTime() - dStartSend;

		if( OGGetAbsoluteTime() - dSecond >= 1 )
		{
			nStoreFPS = framesThisSecond;
	 		dStoreSendTimeS = dSendTotal;
			dSecond++;
			dSendTotal = 0;
			framesThisSecond = 0;
		}

		double dStart = OGGetAbsoluteTime();
		double dSecond = dStart;


		char cts[128];
		sprintf( cts, "R: %d\nFPS: %d\nSend %.3fms", r, nStoreFPS, dStoreSendTimeS*1000 );
		CNFGPenX = 220;
		CNFGPenY = 30;
		CNFGDrawText( cts, 2 );
		if( r < 0 ) goto reconnect;
	}

	hid_close( hd );
	return 0;
}

