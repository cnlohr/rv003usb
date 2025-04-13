// Could be defined here, or in the processor defines.
#define SYSTEM_CORE_CLOCK 48000000
#define SYSTICK_USE_HCLK

#include "ch32fun.h"
#include <stdio.h>
#include <string.h>

#define INSTANCE_DESCRIPTORS
#include "rv003usb.h"

// These are required to be able to compare PORTs against each other
#define PORTIDA 0
#define PORTIDC 2
#define PORTIDD 3
#define LOCAL_CONCAT(A, B) A##B
#define LOCAL_EXP(A, B) LOCAL_CONCAT(A,B)
#define PORTID_EQUALS(A, B) (LOCAL_CONCAT(PORTID,A) == LOCAL_CONCAT(PORTID,B))

// To use time debugging, enable thsi here, and RV003USB_DEBUG_TIMING in the .S
// You must update in tandem
//#define RV003USB_DEBUG_TIMING 1

// If you don't want to automatically boot into the application ever,
// comment out BOOTLOADER_TIMEOUT_USB and set this flag:
//#define DISABLE_BOOTLOAD

// Set this if you want the bootloader to keep the Port configuration untouched
// Otherwise it will reset all Pins on the Port used to input; costs 8-16 Bytes
//#define BOOTLOADER_KEEP_PORT_CFG

// Bootloader Button Config
// If you want to use a Button during boot to enter bootloader, use these defines 
// to setup the Button. If you do, it makes sense to also set DISABLE_BOOTLOAD above, 
// set BOOTLOADER_TIMEOUT_PWR to 0 and disable BOOTLOADER_TIMEOUT_USB
//#define BOOTLOADER_BTN_PORT D
//#define BOOTLOADER_BTN_PIN 2
//#define BOOTLOADER_BTN_TRIG_LEVEL 0 // 1 = HIGH; 0 = LOW
//#define BOOTLOADER_BTN_PULL 1 // 1 = Pull-Up; 0 = Pull-Down; Optional, comment out for floating input

// Timeout for bootloader after power-up, set to 0 to stay in bootloader forever
// 75ms per unit; 67 ~= 5s
#define BOOTLOADER_TIMEOUT_PWR 67

// Timeout (reset) for bootloader once USB Host is detected, set to 0 to stay in bootloader forever
// 75ms per unit; 0 costs 28 Bytes, >0 costs 48 Bytes; Comment out if not used
//#define BOOTLOADER_TIMEOUT_USB 0

// Timeout Timebase; Careful: Constant works out to a single lbu instruction. Changes may result in bigger code
// -0x100000 = 302ms; -0x040000 = 75ms;
#define BOOTLOADER_TIMEOUT_BASE -0x40000

// Uncomment to be able to reboot into bootloader from firmware without pressing a button
// For this to work add a following reboot function to your firmware code:
// ----------------------------------
// FLASH->BOOT_MODEKEYR = 0x45670123;
// FLASH->BOOT_MODEKEYR = 0xCDEF89AB;
// FLASH->STATR = 0x4000;
// RCC->RSTSCKR |= 0x1000000;
// PFIC->CFGR = 0xBEEF0080;
// ----------------------------------
// #define SOFT_REBOOT_TO_BOOTLOADER

#define SCRATCHPAD_SIZE 128
extern volatile int32_t runwordpad;
static uint32_t runwordpadready = 0;
extern uint8_t scratchpad[SCRATCHPAD_SIZE];

#ifdef BOOTLOADER_TIMEOUT_USB
volatile uint8_t reset_timeout = 0;
#endif

struct rv003usb_internal rv003usb_internal_data;

// This is the data actually required for USB.
uint8_t data_receptive;

void SystemInit48HSIUNSAFE( void );

static inline void asmDelay(int delay) {
	asm volatile(
"1:	c.addi %[delay], -1\n"
"bne %[delay], x0, 1b\n" :[delay]"+r"(delay)  );
}

#ifndef USB_PIN_DPU
extern uint32_t _boot_firmware_xor;
uint32_t secret_xor __attribute__((section(".secret_address"))) __attribute__((used)) = (uint32_t)(&_boot_firmware_xor);
// noreturn attribute saves 2-4 bytes. We can use it because we reboot the chip at the end of this function
void boot_usercode() __attribute__((section(".boot_firmware"))) __attribute__((noinline, noreturn));
#else
uint32_t secret_xor __attribute__((section(".secret_address"))) __attribute__((used)) = 0;
#endif

void boot_usercode()
{
#ifdef BOOTLOADER_DEBUG_BOOT
	GPIOC->CFGLR = (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*0);
	GPIOC->BSHR = 1<<(0);
#endif
#ifndef USB_PIN_DPU
	LOCAL_EXP(GPIO,USB_PORT)->CFGLR = (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*USB_PIN_DM);
	LOCAL_EXP(GPIO,USB_PORT)->BSHR = 1<<(USB_PIN_DM + 16);
	asmDelay(1000000);
#endif
	FLASH->BOOT_MODEKEYR = FLASH_KEY1;
	FLASH->BOOT_MODEKEYR = FLASH_KEY2;
	FLASH->STATR = 0; // 1<<14 is zero, so, boot user code.
	// FLASH->CTLR = CR_LOCK_Set;	// Not needed, flash is locked at reboot (soft reboot counts, I checked)
	PFIC->SCTLR = 1<<31;
}

int main()
{
	// In the bootloader, the normal ch32v003fun startup doesn't happen, so to enable the systick, we have to manually enable it,
	// and configure it.  This enables and configures it for high speed.
	SysTick->CTLR = 5;

	// Enable GPIOs, TIMERs
	RCC->APB2PCENR = RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM1 | RCC_APB2Periph_GPIOA  | RCC_APB2Periph_AFIO | RCC_APB2Periph_TIM1;

#if defined( RV003USB_DEBUG_TIMING ) && RV003USB_DEBUG_TIMING
	// GPIO C0 Push-Pull
	GPIOC->CFGLR = (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*0) |
	               (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF)<<(4*3) | // PC3 = T1C3
	               (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF)<<(4*4) | // PC4 = T1C4
	               (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*2);
#endif

#if defined( RV003USB_DEBUG_TIMING ) && RV003USB_DEBUG_TIMING
	{
		// PC4 is MCO (for watching timing)
		GPIOC->CFGLR &= ~(GPIO_CFGLR_MODE4 | GPIO_CFGLR_CNF4);
		GPIOC->CFGLR |= GPIO_CFGLR_CNF4_1 | GPIO_CFGLR_MODE4_0 | GPIO_CFGLR_MODE4_1;
		RCC->CFGR0 = (RCC->CFGR0 & ~RCC_CFGR0_MCO) | RCC_CFGR0_MCO_SYSCLK;

		// PWM is used for debug timing. 
		TIM1->PSC = 0x0000;

		// Auto Reload - sets period
		TIM1->ATRLR = 0xffff;

		// Reload immediately
		TIM1->SWEVGR |= TIM_UG;

		// Enable CH4 output, positive pol
		TIM1->CCER |= TIM_CC3E | TIM_CC3NP;

		// CH2 Mode is output, PWM1 (CC1S = 00, OC1M = 110)
		TIM1->CHCTLR2 |= TIM_OC3M_2 | TIM_OC3M_1;

		// Set the Capture Compare Register value to 50% initially
		TIM1->CH3CVR = 2;
		
		// Enable TIM1 outputs
		TIM1->BDTR |= TIM_MOE;
		
		// Enable TIM1
		TIM1->CTLR1 |= TIM_CEN;
	}
#endif

#if defined(BOOTLOADER_BTN_PORT) && defined(BOOTLOADER_BTN_PULL)
	#if BOOTLOADER_BTN_PULL == 1
		LOCAL_EXP(GPIO,BOOTLOADER_BTN_PORT)->BSHR = 1<<BOOTLOADER_BTN_PIN;
	#else
		LOCAL_EXP(GPIO,BOOTLOADER_BTN_PORT)->BSHR = 1<<(BOOTLOADER_BTN_PIN+16);
	#endif
#endif


#if defined(BOOTLOADER_BTN_PORT) && !PORTID_EQUALS(BOOTLOADER_BTN_PORT, USB_PORT)
	// GPIO setup for Bootloader Button PORT
	LOCAL_EXP(GPIO,BOOTLOADER_BTN_PORT)->CFGLR = (
#ifdef BOOTLOADER_KEEP_PORT_CFG
		LOCAL_EXP(GPIO,BOOTLOADER_BTN_PORT)->CFGLR 
#else
		0x44444444 // reset value (all input)
#endif
		// Reset the Bootloader Pin
		& ~( 0xF<<(4*BOOTLOADER_BTN_PIN) )
	) |
	// Configure the Bootloader Pin
	#if defined(BOOTLOADER_BTN_PULL)
		(GPIO_Speed_In | GPIO_CNF_IN_PUPD)<<(4*BOOTLOADER_BTN_PIN);
	#else
		(GPIO_Speed_In | GPIO_CNF_IN_FLOATING)<<(4*BOOTLOADER_BTN_PIN);
	#endif
#endif
		
	// GPIO setup.
	LOCAL_EXP(GPIO,USB_PORT)->CFGLR = (
#ifdef BOOTLOADER_KEEP_PORT_CFG
		LOCAL_EXP(GPIO,USB_PORT)->CFGLR 
#else
		0x44444444 // reset value (all input)
#endif
		// Reset the USB Pins
		& ~( 0xF<<(4*USB_PIN_DP) | 0xF<<(4*USB_PIN_DM) 
		#if defined(USB_PIN_DPU) && (PORTID_EQUALS(USB_DPU_PORT,USB_PORT) || !defined(USB_DPU_PORT))
			| 0xF<<(4*USB_PIN_DPU)
		#endif
		// reset Bootloader Btn Pin
		#if defined(BOOTLOADER_BTN_PORT) && PORTID_EQUALS(BOOTLOADER_BTN_PORT,USB_PORT)
			| 0xF<<(4*BOOTLOADER_BTN_PIN)
		#endif
		)
	) |
	// Configure the USB Pins
#ifdef USB_PIN_DPU
		(GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*USB_PIN_DPU) |
#endif
#if defined(BOOTLOADER_BTN_PORT) && PORTID_EQUALS(BOOTLOADER_BTN_PORT,USB_PORT)
	#if defined(BOOTLOADER_BTN_PULL)
		(GPIO_Speed_In | GPIO_CNF_IN_PUPD)<<(4*BOOTLOADER_BTN_PIN) | 
	#else
		(GPIO_Speed_In | GPIO_CNF_IN_FLOATING)<<(4*BOOTLOADER_BTN_PIN) | 
	#endif
#endif
		(GPIO_Speed_In | GPIO_CNF_IN_PUPD)<<(4*USB_PIN_DP) | 
		(GPIO_Speed_In | GPIO_CNF_IN_PUPD)<<(4*USB_PIN_DM);

#if defined(USB_DPU_PORT) && !PORTID_EQUALS(USB_DPU_PORT,USB_PORT)
	LOCAL_EXP(GPIO, USB_DPU_PORT)->CFGLR &= ~(0xf<<(4*USB_PIN_DPU));
   LOCAL_EXP(GPIO, USB_DPU_PORT)->CFGLR |= (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*USB_PIN_DPU);
#endif

	// Configure USB_PIN_DM (D-) as an interrupt on falling edge.
	AFIO->EXTICR = LOCAL_EXP(GPIO_PortSourceGPIO,USB_PORT)<<(USB_PIN_DM*2); // Configure EXTI interrupt for USB_PIN_DM
	EXTI->INTENR = 1<<USB_PIN_DM; // Enable EXTI interrupt
	EXTI->FTENR = 1<<USB_PIN_DM;  // Enable falling edge trigger for USB_PIN_DM (D-)

#if defined(BOOTLOADER_BTN_PORT) && defined(BOOTLOADER_BTN_TRIG_LEVEL) && defined(BOOTLOADER_BTN_PIN)
	#if BOOTLOADER_BTN_TRIG_LEVEL == 0
    #if defined(SOFT_REBOOT_TO_BOOTLOADER)
      if(LOCAL_EXP(GPIO,BOOTLOADER_BTN_PORT)->INDR & (1<<BOOTLOADER_BTN_PIN) && !(RCC->RSTSCKR == 0x10000000)) boot_usercode();
    #else
		  if(LOCAL_EXP(GPIO,BOOTLOADER_BTN_PORT)->INDR & (1<<BOOTLOADER_BTN_PIN)) boot_usercode();
    #endif
	#else
    #if defined(SOFT_REBOOT_TO_BOOTLOADER)
		  if((LOCAL_EXP(GPIO,BOOTLOADER_BTN_PORT)->INDR & (1<<BOOTLOADER_BTN_PIN)) == 0 && !(RCC->RSTSCKR == 0x10000000)) boot_usercode();
    #else
      if((LOCAL_EXP(GPIO,BOOTLOADER_BTN_PORT)->INDR & (1<<BOOTLOADER_BTN_PIN)) == 0) boot_usercode();
    #endif
	#endif
#endif

#if defined(USB_PIN_DPU) && !defined(USB_DPU_PORT)
	// This drives USB_PIN_DPU (D- Pull-Up) high, which will tell the host that we are going on-bus.
	LOCAL_EXP(GPIO,USB_PORT)->BSHR = 1<<USB_PIN_DPU;
#endif

#if defined(USB_PIN_DPU) && defined(USB_DPU_PORT)
	LOCAL_EXP(GPIO,USB_DPU_PORT)->BSHR = 1<<USB_PIN_DPU;
#endif

	// enable interrupt
	NVIC_EnableIRQ( EXTI7_0_IRQn );

	// Bootloader timeout / localpad: 
	// localpad counting up to 0 is used for timeout
	// localpad transitioning from -1 to 0 boots user code
	// localpad set to 0 disables timeout
	// localpad counting down to 0 is used for executing code from scratchpad
	int32_t localpad = BOOTLOADER_TIMEOUT_BASE * BOOTLOADER_TIMEOUT_PWR;
	while(1)
	{
#if !(BOOTLOADER_TIMEOUT_PWR == 0)
		if( localpad < 0 )
		{
			if( ++localpad == 0 )
			{
				// Boot to user program.
#ifndef DISABLE_BOOTLOAD
				FLASH->BOOT_MODEKEYR = FLASH_KEY1;
				FLASH->BOOT_MODEKEYR = FLASH_KEY2;
				FLASH->STATR = 0; // 1<<14 is zero, so, boot user code.
				FLASH->CTLR = CR_LOCK_Set;
				PFIC->SCTLR = 1<<31;
#endif
			}
#ifdef BOOTLOADER_TIMEOUT_USB
			// Bootloader timeout reset was requested after first USB Communication
	#if BOOTLOADER_TIMEOUT_USB > 0
			if(reset_timeout == 1) {
				localpad = BOOTLOADER_TIMEOUT_BASE * BOOTLOADER_TIMEOUT_USB;
				reset_timeout = 2; // prevent from triggering again
			}
	#else
			if(reset_timeout) localpad = 0;
	#endif
#endif
		}
#endif
		if( localpad > 0 )
		{
			if( --localpad == 0 )
			{
				/* Scratchpad strucure:
					4-bytes:		LONG( 0x000000aa )
						... code (this is executed) (120 bytes)
					4-bytes:        LONG( 0x1234abcd )

					After the scratchpad is the runpad, its structure is:
					4-bytes:   int32_t  if negative, how long to go before bootloading.  If 0, do nothing.  If positive, execute.
						... 1kB of totally free space.
				*/
				typedef void (*setype)( uint32_t *, volatile int32_t * );
				setype scratchexec = (setype)(scratchpad+4);
				scratchexec( (uint32_t*)&scratchpad[0], &runwordpad );
			}
		}

		uint32_t commandpad = runwordpad;
		if( commandpad )
		{
			localpad = commandpad-1;
			runwordpad = 0;
		}
	}
}

#define ENDPOINT0_SIZE 8 //Fixed for USB 1.1, Low Speed.

void usb_pid_handle_in( uint32_t addr, uint8_t * data, uint32_t endp, uint32_t unused, struct rv003usb_internal * ist )
{
	ist->current_endpoint = endp;
	struct usb_endpoint * e = &ist->eps[endp];

	int tosend = 0;
	const uint8_t * sendnow;
	uint8_t sendtok = e->toggle_in?0b01001011:0b11000011;
	
	// Handle IN (sending data back to PC)
	// Do this down here.
	// We do this because we are required to have an in-endpoint.  We don't
	// have to do anything with it, though.
	if( endp ) //XXX TODO: This can be reworked - if it's anything other than "is_descriptor" then send nak.
	{
		// Save some space
		goto sendempty;
		return;
	}

	const uint8_t * tsend = e->opaque;

	int offset = (e->count)<<3;
	tosend = e->max_len - offset;
	if( tosend > ENDPOINT0_SIZE ) tosend = ENDPOINT0_SIZE;
	if( tosend < 0 ) tosend = 0;
	sendnow = tsend + offset;

	// DON'T start the execution timer until after we receive the IN from the usb host req.
	if (runwordpadready)
	{
		runwordpad = sendtok; // Anything non-zero is fine.
		runwordpadready = 0;
	}

	if( !tosend || !tsend )
	{
sendempty:
		usb_send_empty( sendtok );
	}
	else
	{
		usb_send_data( sendnow, tosend, 0, sendtok );
	}
	return;
}

void usb_pid_handle_data( uint32_t this_token, uint8_t * data, uint32_t which_data, uint32_t length, struct rv003usb_internal * ist )
{
#ifdef BOOTLOADER_TIMEOUT_USB
	// Detect first USB communication in order to reset the Bootloader Timeout if configured
	#if BOOTLOADER_TIMEOUT_USB > 0
		if(reset_timeout == 0) reset_timeout = 1;
	#else
		reset_timeout = 1;
	#endif
#endif

	//Received data from host.
	int cep = ist->current_endpoint;
	struct usb_endpoint * e = &ist->eps[cep];

	// Alrady received this packet.
	if( e->toggle_out != which_data )
	{
		goto just_ack;
	}

	if( cep == 0 )
	{
		e->toggle_out = !e->toggle_out;
		if( ist->setup_request )
		{
			struct usb_urb * s = __builtin_assume_aligned( (struct usb_urb *)(data), 4 );

			uint32_t wvi = s->lValueLSBIndexMSB;
			//Send just a data packet.
			e->count = 0;
			e->opaque = 0;
			ist->setup_request = 0;
			e->max_len = 0;

			if( s->wRequestTypeLSBRequestMSB == 0x01a1 )
			{
				// Class read request.
				uint32_t wlen = s->wLength;
				if( wlen > sizeof(scratchpad) ) wlen = SCRATCHPAD_SIZE;
				// The host wants to read back from us.
				e->max_len = wlen;
				e->opaque = scratchpad;
			}
			else if( s->wRequestTypeLSBRequestMSB == 0x0921 )
			{
				// Class write request.
				e->max_len = SCRATCHPAD_SIZE;
				runwordpad = 1; //request stoppage.
				e->opaque = scratchpad;
			}
			else if( (s->wRequestTypeLSBRequestMSB & 0xff80) == 0x0680 )
			{
				int i;
				const struct descriptor_list_struct * dl;
				for( i = 0; i < DESCRIPTOR_LIST_ENTRIES; i++ )
				{
					dl = &descriptor_list[i];
					if( dl->lIndexValue == wvi )
					{
						// Send back descriptor.
						e->opaque = (uint8_t*)dl->addr;
						uint16_t swLen = s->wLength;
						uint16_t elLen = dl->length;
						e->max_len = (swLen < elLen)?swLen:elLen;
						break;
					}
				}

				goto just_ack;
			}
			else if( s->wRequestTypeLSBRequestMSB == 0x0500 )
			{
				//Set address.
				ist->my_address = wvi;
			}
			else
			{
			}
		}
		else if( e->opaque )
		{
			// Continuing data.
			uint8_t * start = &e->opaque[e->count<<3];
			int l = length-3;
			int i;
			for( i = 0; i < l; i++ )
				start[i] = data[i];//((intptr_t)data)>>(i*8);
			e->count ++;

			if( start + length - 3 >= scratchpad + SCRATCHPAD_SIZE )
			{
				// If the last 4 bytes are 0x1234abcd, then we can go!
				uint32_t * last4 = (uint32_t*)(start + 4);					
				if( *last4 == 0x1234abcd )
				{
					*last4 = 0;
					runwordpadready = 1; // Request exectution, but wait for another IN pkt.
				}
				e->opaque = 0;
			}
			// Allow user code to receive data.
		}
	}
just_ack:
	{
		//Got the right data.  Acknowledge.
		usb_send_data( 0, 0, 2, 0xD2 ); // Send ACK
	}
	return;
}




