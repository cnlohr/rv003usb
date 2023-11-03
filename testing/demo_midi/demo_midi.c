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
void midi_send(uint8_t * msg, uint8_t len)
{
	memcpy( midi_in.buffer, msg, len );
	midi_in.len = len;
}
#define midi_send_ready() (midi_in.len == 0)

// Set up PC2 to PC5 as inputs
void keyboard_init( void )
{
	// Enable GPIOC
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;

	// Clear old values
	GPIOC->CFGLR &= ~(0xf<<(4*2) | 0xf<<(4*3) | 0xf<<(4*4) | 0xf<<(4*5));

	// Set up C2, C3, C4, C5 as input with pull-up/down
	GPIOC->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_PUPD) << (4*2) |
					(GPIO_Speed_In | GPIO_CNF_IN_PUPD) << (4*3) |
					(GPIO_Speed_In | GPIO_CNF_IN_PUPD) << (4*4) |
					(GPIO_Speed_In | GPIO_CNF_IN_PUPD) << (4*5);

	// Set output high for pull-up
	GPIOC->BSHR = GPIO_BSHR_BS2 |
				  GPIO_BSHR_BS3 |
				  GPIO_BSHR_BS4 |
				  GPIO_BSHR_BS5;

}

void scan_keyboard(void)
{
	static uint8_t keystate = 0;
	uint8_t midi_msg[4] = {0x09, 0x90, 0x40, 0x7F}; // note on
	uint16_t input = GPIOC->INDR;

	// check keys on pins C2 to C5
	for (uint8_t i = 2; i <= 5; i++) {
		uint8_t mask = (1<<i);

		if ((input & mask) != (keystate & mask)) {
			// note number
			midi_msg[2] = 0x40+i;
			if (input & mask) {
				midi_msg[3] = 0x00; // note off
			} else {
				midi_msg[3] = 0x7F; // note on
			}
			while (!midi_send_ready()) {};
			midi_send(midi_msg, 4);
		}
	}
	keystate = input;
}


// Output a variable-period square wave on PC0 and PC1
void tim2_init( void )
{
	// Enable GPIOC and TIM2
	RCC->APB1PCENR |= RCC_APB1Periph_TIM2;
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;

	// PC0 is T2CH3, 10MHz Output alt func, push-pull
	GPIOC->CFGLR &= ~(0xf<<(4*0));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF)<<(4*0);

	// PC1 is T2CH1 alternate
	GPIOC->CFGLR &= ~(0xf<<(4*1));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF)<<(4*1);

	// Provide clock to AFIO peripheral
	RCC->APB2PCENR |= RCC_APB2Periph_AFIO;
	// Remap T2CH1 to PC1, leave T2CH3 as C0
	AFIO->PCFR1 |= AFIO_PCFR1_TIM2_REMAP_PARTIALREMAP2;

	// Set prescaler
	TIM2->PSC = 0x0008;

	// CTLR1: default is up, events generated, edge align
	// enable auto-reload of preload
	TIM2->CTLR1 |= TIM_ARPE;

	// Set TIM2CH1 to output, flip mode, preload enable 
	TIM2->CHCTLR1 |= TIM_OC1M_1 | TIM_OC1M_0 | TIM_OC1PE; 

	// same for TIM2CH3
	TIM2->CHCTLR2 |= TIM_OC3M_1 | TIM_OC3M_0 | TIM_OC3PE; 

	// Enable Channel outputs, set CC1 to inverted polarity
	TIM2->CCER |= TIM_CC1E | TIM_CC3E | TIM_CC1P;

	// Compare/capture registers to zero
	TIM2->CH1CVR = 0;
	TIM2->CH3CVR = 0;

}

void tim2_set_period(uint16_t period)
{
	if (period) {
		// Auto reload value
		TIM2->ATRLR = period;
		// Timer enable
		TIM2->CTLR1 |= TIM_CEN;
	} else {
		TIM2->CTLR1 &= ~TIM_CEN;
	}
}

// 2**( 16 - n/12 )
const uint16_t noteLookup[128] = { 65535,61856,58385,55108,52015,49096,46340,43739,41284,38967,36780,34716,32767,30928,29192,27554,26007,24548,23170,21870,20642,19484,18390,17358,16384,15464,14596,13777,13004,12274,11585,10935,10321,9742,9195,8679,8192,7732,7298,6888,6502,6137,5792,5467,5161,4871,4598,4339,4096,3866,3649,3444,3251,3068,2896,2734,2580,2435,2299,2170,2048,1933,1825,1722,1625,1534,1448,1367,1290,1218,1149,1085,1024,967,912,861,813,767,724,683,645,609,575,542,512,483,456,431,406,384,362,342,323,304,287,271,256,242,228,215,203,192,181,171,161,152,144,136,128,121,114,108,102,96,91,85,81,76,72,68,64,60,57,54,51,48,45,43 };

// midi data from host -> device (OUT)
void midi_receive(uint8_t * msg)
{
	static uint8_t note;

	if (msg[1] == 0x90 && msg[3] !=0) { // Note-on, velocity nonzero
		note = msg[2];
		tim2_set_period( noteLookup[ note ] );
	}

	// Note-off or note-on with zero velocity
	else if (msg[2] == note && (msg[1]==0x80 || (msg[1]==0x90 && msg[3]==0)) ) {
		tim2_set_period(0);
	}
}

int main()
{
	SystemInit();
	keyboard_init();
	tim2_init();
	usb_setup();
	while(1)
	{
		scan_keyboard();
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




