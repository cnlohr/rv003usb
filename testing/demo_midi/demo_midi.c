#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"

typedef struct {
	volatile uint8_t len;
	uint8_t buffer[8];
} midi_message_t;

midi_message_t midi_in;

// midi data from device -> host (IN)
void midi_send(uint8_t * msg, uint8_t len){
	memcpy( midi_in.buffer, msg, len );
	midi_in.len = len;
}
#define midi_send_ready() (midi_in.len == 0)

// midi data from host -> device (OUT)
void midi_receive(uint8_t * msg){

	printf("Recv %02x %02x %02x %02x\n", msg[0], msg[1], msg[2], msg[3]);

}


int main()
{
	SystemInit();
	usb_setup();
	while(1)
	{

		Delay_Ms(100);

		if (midi_send_ready()) {
			uint8_t midi[4] = {0x09, 0x90, 0x40, 0x7F}; // note on
			midi_send(midi, 4);
		}

		Delay_Ms(100);

		if (midi_send_ready()) {
			uint8_t midi[4] = {0x09, 0x90, 0x40, 0x00}; // note off
			midi_send(midi, 4);
		}
	}
}

void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	if (endp && midi_in.len) {
		usb_send_data( midi_in.buffer, midi_in.len, 0, sendtok );
		midi_in.len = 0;
	} else {
		usb_send_empty( sendtok );
	}
}

void usb_handle_other_control_message( struct usb_endpoint * e, struct usb_urb * s, struct rv003usb_internal * ist )
{
	LogUEvent( SysTick->CNT, s->wRequestTypeLSBRequestMSB, s->lValueLSBIndexMSB, s->wLength );
}

void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
	if (len) midi_receive(data);
	if (len==8) midi_receive(&data[4]);
}




