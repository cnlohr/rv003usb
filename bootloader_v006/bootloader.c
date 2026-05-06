#include "ch32fun.h"
#include <stdio.h>
#include <string.h>

#define INSTANCE_DESCRIPTORS
#include "rv003usb.h"

#define PORTIDA 0
#define PORTIDB 1
#define PORTIDC 2
#define PORTIDD 3
#define LOCAL_CONCAT(A, B) A##B
#define LOCAL_EXP(A, B) LOCAL_CONCAT(A,B)

// If you don't want to automatically boot into the application ever,
// comment out BOOTLOADER_TIMEOUT_USB and set this flag:
#define DISABLE_BOOTLOAD 0

#define BOOTLOADER_LED_PIN PC0
#define BOOTLOADER_LED_POLARITY 1
#ifdef BOOTLOADER_LED_PIN
#define BOOT_LED(n) funDigitalWrite(BOOTLOADER_LED_PIN, n)
#else
#define BOOT_LED(n)
#endif

// Bootloader Button Config
// If you want to use a Button during boot to enter bootloader, use these defines 
// to setup the Button. If you do, it makes sense to also set DISABLE_BOOTLOAD above, 
// set BOOTLOADER_TIMEOUT_PWR_MS to 0 and disable BOOTLOADER_TIMEOUT_USB
#define BOOTLOADER_BTN_PIN PD7
#define BOOTLOADER_BTN_TRIG_LEVEL 0 // 1 = HIGH; 0 = LOW
#define BOOTLOADER_BTN_PULL 1 // 1 = Pull-Up; 0 = Pull-Down; Optional, comment out for floating input

// Timeout for bootloader after power-up, set to 0 to stay in bootloader forever
#define BOOTLOADER_TIMEOUT_PWR_MS 5000

#define BOOTLOADER_TIMEOUT_BASE 4294967295 - Ticks_from_Ms(BOOTLOADER_TIMEOUT_PWR_MS)

// Uncomment to be able to reboot into bootloader from firmware without pressing a button
// For this to work add a following reboot function to your firmware code:
// ----------------------------------
// FLASH->BOOT_MODEKEYR = 0x45670123;
// FLASH->BOOT_MODEKEYR = 0xCDEF89AB;
// FLASH->STATR = 0x4000;
// RCC->RSTSCKR |= 0x1000000;
// PFIC->CFGR = 0xBEEF0080;
// ----------------------------------
#define SOFT_REBOOT_TO_BOOTLOADER

#if CH32V002
#define DATA_SIZE 2048
#elif CH32V004 || CH32V005
#define DATA_SIZE 4096
#else
#define DATA_SIZE 6144
#endif

#define SCRATCHPAD_SIZE DATA_SIZE+128

volatile int32_t runwordpad;
uint32_t runwordpadready = 0;
uint8_t scratchpad[SCRATCHPAD_SIZE];
uint32_t current_scratchpad_size = 128;

volatile uint8_t reset_timeout = 0;

static inline void asmDelay(int delay) {
	asm volatile(
"1:	c.addi %[delay], -1\n"
"bne %[delay], x0, 1b\n" :[delay]"+r"(delay)  );
}

// #ifndef USB_PIN_DPU
// // noreturn attribute saves 2-4 bytes. We can use it because we reboot the chip at the end of this function
// void boot_usercode() __attribute__((section(".boot_firmware"))) __attribute__((noinline, noreturn));
// #else
// #endif

void boot_usercode()
{
#ifndef USB_PIN_DPU
	LOCAL_EXP(GPIO,USB_PORT)->CFGLR = (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*USB_PIN_DM);
	LOCAL_EXP(GPIO,USB_PORT)->BSHR = 1<<(USB_PIN_DM + 16);
	asmDelay(1000000);
#else
	LOCAL_EXP(GPIO,USB_PORT)->BSHR = 1<<(USB_PIN_DPU + 16);
	asmDelay(1000000);
#endif
	FLASH->BOOT_MODEKEYR = FLASH_KEY1;
	FLASH->BOOT_MODEKEYR = FLASH_KEY2;
	FLASH->STATR = 0; // 1<<14 is zero, so, boot user code.
	// FLASH->CTLR = CR_LOCK_Set;    // Not needed, flash is locked at reboot (soft reboot counts, I checked)
	PFIC->SCTLR = 1<<31;
}

int main()
{
	SystemInit();
	usb_setup();
	SysTick->CNT = BOOTLOADER_TIMEOUT_BASE;
	// Enable GPIOs, TIMERs
	RCC->APB2PCENR = RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM1 | RCC_APB2Periph_GPIOA  | RCC_APB2Periph_AFIO | RCC_APB2Periph_TIM1;

#if defined(BOOTLOADER_LED_PIN)
	funPinMode(BOOTLOADER_LED_PIN, (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP));
#endif

#if defined(BOOTLOADER_BTN_PIN) && defined(BOOTLOADER_BTN_TRIG_LEVEL)

	// Configure the Bootloader Pin
	#if defined(BOOTLOADER_BTN_PULL)
		funPinMode(BOOTLOADER_BTN_PIN, GPIO_Speed_In | GPIO_CNF_IN_PUPD );
	#else
		funPinMode(BOOTLOADER_BTN_PIN, GPIO_Speed_In | GPIO_CNF_IN_FLOATING);
	#endif
	
	#if defined(BOOTLOADER_BTN_PULL)
	#if BOOTLOADER_BTN_PULL == 1
		funDigitalWrite(BOOTLOADER_BTN_PIN, 1);
	#else
		funDigitalWrite(BOOTLOADER_BTN_PIN, 0);
	#endif
	#endif

	asmDelay(1000000);

	#if !defined(DISABLE_BOOTLOAD) || !DISABLE_BOOTLOAD
	#if BOOTLOADER_BTN_TRIG_LEVEL == 0
		#if defined(SOFT_REBOOT_TO_BOOTLOADER)
			if(funDigitalRead(BOOTLOADER_BTN_PIN) && !(RCC->RSTSCKR == 0x10000000)) boot_usercode();
		#else
			if(funDigitalRead(BOOTLOADER_BTN_PIN)) boot_usercode();
		#endif
	#else
		#if defined(SOFT_REBOOT_TO_BOOTLOADER)
			if(!funDigitalRead(BOOTLOADER_BTN_PIN) && !(RCC->RSTSCKR == 0x10000000)) boot_usercode();
		#else
			if(!funDigitalRead(BOOTLOADER_BTN_PIN)) boot_usercode();
		#endif
	#endif
	#endif
#endif

	// Bootloader timeout / localpad: 
	// localpad counting up to 0 is used for timeout
	// localpad transitioning from -1 to 0 boots user code
	// localpad set to 0 disables timeout
	// localpad counting down to 0 is used for executing code from scratchpad
	int32_t localpad = (int32_t)SysTick->CNT;
	while(1)
	{
#if defined(BOOTLOADER_TIMEOUT_PWR_MS) && BOOTLOADER_TIMEOUT_PWR_MS
		if( localpad < 0 )
		{
			BOOT_LED(1);
			localpad = (int32_t)SysTick->CNT;
			if( localpad >= 0 )
			{
				BOOT_LED(0);
				// Boot to user program.
#if !defined(DISABLE_BOOTLOAD) || !DISABLE_BOOTLOAD
				boot_usercode();
#endif
			}
			if(reset_timeout) localpad = 0;
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
				BOOT_LED(1);
			}
		}

		uint32_t commandpad = runwordpad;
		if( commandpad )
		{
			BOOT_LED(0);
			localpad = commandpad-1;
			runwordpad = 0;
		}
	}
}

void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	// Make sure we only deal with control messages.  Like get/set feature reports.
	if( endp )
	{
		usb_send_empty( sendtok );
	}
}

void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
	if( e->opaque )
	{
		uint8_t * start = &e->opaque[e->count<<3];
		for( int i = 0; i < len; i++ )
			start[i] = data[i];//((intptr_t)data)>>(i*8);
		e->count ++;

		// If the last 4 bytes are 0x1234abcd, then we can go!
		uint32_t * last4 = (uint32_t*)(start + 4);
		if( *last4 == 0x1234abcd )
		{
			*last4 = 0;
			runwordpadready = 1; // Request execution, but wait for another IN pkt.
			e->opaque = 0;
		}
		
		if( e->count >= SCRATCHPAD_SIZE/8 ) e->opaque = 0;
	}
}

void usb_handle_hid_get_report_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
	// Class read request.
	if( reqLen > SCRATCHPAD_SIZE ) reqLen = SCRATCHPAD_SIZE;
	// The host wants to read back from us.
	e->max_len = reqLen;
	e->opaque = scratchpad;
}

void usb_handle_hid_set_report_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
	// Class write request.
	e->max_len = SCRATCHPAD_SIZE;
	runwordpad = 1; //request stoppage.
	e->opaque = scratchpad;
}

void usb_handle_other_control_message( struct usb_endpoint * e, struct usb_urb * s, struct rv003usb_internal * ist )
{
}
