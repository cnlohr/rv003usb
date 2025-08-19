#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"
#include "ch32v003_GPIO_branchless.h"
#define WS2812DMA_IMPLEMENTATION
//#define WSRBG //For WS2816C's.
#define WSGRB // For SK6805-EC15
#define NR_LEDS 4

#include "ws2812b_dma_spi_led_driver.h"

#include "color_utilities.h"


uint16_t phases[NR_LEDS];
int frameno;
volatile int tween = -NR_LEDS;
uint8_t rgbmode = 0;

uint32_t WS2812BLEDCallback( int ledno )
{
	uint8_t index = (phases[ledno])>>8;
	uint8_t rsbase = sintable[index];
	uint8_t rs = rsbase>>3;
	uint32_t fire = ((huetable[(rs+190)&0xff]>>1)<<16) | (huetable[(rs+30)&0xff]) | ((huetable[(rs+0)]>>1)<<8);
	uint32_t ice  = 0x7f0000 | ((rsbase>>1)<<8) | ((rsbase>>1));
    if(rgbmode ==0){
        fire = 0x0;
        ice = 0x0;
    }
    if(rgbmode ==1){
        fire = 0xffffffff;
        ice = 0xffffffff;
    }
    int16_t a = tween + ledno;
    if (a < 0)   a = 0;
    if (a > 255) a = 255;

    return TweenHexColors(fire, ice, (uint8_t)a);
}

void FlashOptionData(uint8_t data0, uint8_t data1) {
	volatile uint16_t hold[6]; 		// array to hold current values while erasing

	// The entire 64 byte data block of the "User-selected words" will be erased
	// so we need to keep a copy of the content for re-writing after erase.
	// Save a few (20) bytes code space by moving 32 bits at a time.
	// 			hold[0]=OB->RDPR;
	// 			hold[1]=OB->USER;
	// 			hold[2]=data0;
	// 			hold[3]=data1;
	// 			hold[4]=OB->WRPR0;
	// 			hold[5]=OB->WRPR1;
	uint32_t *hold32p=(uint32_t *)hold;
	uint32_t *ob32p=(uint32_t *)OB_BASE;
	hold32p[0]=ob32p[0]; 			// Copy RDPR and USER
	hold32p[1]=data0+(data1<<16);	// Copy in the two Data values to be written
	hold32p[2]=ob32p[2];			// Copy WRPR0 and WEPR1

	// Unlock both the general Flash and the User-selected words
	FLASH->KEYR = FLASH_KEY1;
	FLASH->KEYR = FLASH_KEY2;
	FLASH->OBKEYR = FLASH_KEY1;
	FLASH->OBKEYR = FLASH_KEY2;

	FLASH->CTLR |= CR_OPTER_Set;			// OBER RW Perform user-selected word erasure	
	FLASH->CTLR |= CR_STRT_Set;    			// STRT RW1 Start. Set 1 to start an erase action,hw automatically clears to 0
	while (FLASH->STATR & FLASH_BUSY);		// Wait for flash operation to be done
	FLASH->CTLR &= CR_OPTER_Reset; 			// Disable erasure mode	

	// Write the held values back one-by-one
	FLASH->CTLR |= CR_OPTPG_Set;   			// OBG  RW Perform user-selected word programming
	uint16_t *ob16p=(uint16_t *)OB_BASE;
	for (int i=0;i<sizeof(hold)/sizeof(hold[0]); i++) {
		ob16p[i]=hold[i];
		while (FLASH->STATR & FLASH_BUSY);	// Wait for flash operation to be done
	}
	FLASH->CTLR &= CR_OPTPG_Reset;			// Disable programming mode

	FLASH->CTLR|=CR_LOCK_Set;				// Lock flash memories again

	return;
}

#define NUM_COLS 1
#define NUM_ROWS 6
uint8_t gain_now = 2;

// Define Rows (Inputs)
const int rows_ports[NUM_ROWS] = {
    GPIO_port_C, // Row 0 - PC0
    GPIO_port_C, // Row 1 - PC7
    GPIO_port_C, // Row 2 - PC1
    GPIO_port_C, // Row 3 - PC2
    GPIO_port_C, // Row 4 - PC3
    GPIO_port_D, // Row 4 - PD6
};

const uint8_t rows_pins[NUM_ROWS] = {
    0, // PC0
    7, // PC7
    1, // PC1
    2, // PC2
    3, // PC3
    6, // PD6
    };

// Button Matrix State
typedef struct {
    uint8_t measuring[NUM_ROWS][NUM_COLS];
    uint8_t debounced_state[NUM_ROWS][NUM_COLS];
} ButtonMatrix;

ButtonMatrix btn_matrix;


// Debounce Information
#define DEBOUNCE_TIME_MS 50

typedef struct {
    uint32_t last_debounce_time[NUM_ROWS][NUM_COLS];
} DebounceInfo;

DebounceInfo debounce_info;

// Function to Initialize GPIOs
void GPIO_Init_All(void) {
    // Enable GPIO Ports
    GPIO_port_enable(GPIO_port_A);
    GPIO_port_enable(GPIO_port_C);
    GPIO_port_enable(GPIO_port_D);

    for(int row = 0; row < NUM_ROWS; row++) {
        GPIO_pinMode(GPIOv_from_PORT_PIN(rows_ports[row], rows_pins[row]), GPIO_pinMode_I_pullDown, GPIO_Speed_In);
    }

	GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_A,1), GPIO_pinMode_I_analog, GPIO_Speed_In);
	GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_A,2), GPIO_pinMode_I_analog, GPIO_Speed_In);
	GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_C,4), GPIO_pinMode_I_analog, GPIO_Speed_In);
	GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_D,2), GPIO_pinMode_I_analog, GPIO_Speed_In);

	GPIO_ADCinit();    
}

// Initialize Button Matrix State
void ButtonMatrix_Init(ButtonMatrix* matrix) {
    for(int row = 0; row < NUM_ROWS; row++) {
        for(int col = 0; col < NUM_COLS; col++) {
            matrix->measuring[row][col] = 0;
            matrix->debounced_state[row][col] = 1;
        }
    }
}

// Initialize Debounce Info
void Debounce_Init(DebounceInfo* db_info) {
    for(int row = 0; row < NUM_ROWS; row++) {
        for(int col = 0; col < NUM_COLS; col++) {
            db_info->last_debounce_time[row][col] = 0;
        }
    }
}

// Scan the Button Matrix with Debouncing
uint8_t keypressed[8] = {0};
void ButtonMatrix_Scan_Debounced(ButtonMatrix* matrix, DebounceInfo* db_info, uint32_t current_time) {
    for(int col = 0; col < NUM_COLS; col++) {
        // Read all rows
        for(int row = 0; row < NUM_ROWS; row++) {
            uint8_t measured_state = GPIO_digitalRead(GPIOv_from_PORT_PIN(rows_ports[row], rows_pins[row])); // Active high
            if(measured_state != matrix->debounced_state[row][col] && matrix->measuring[row][col] == 1) {
                db_info->last_debounce_time[row][col] = current_time;
                matrix->measuring[row][col] = 1;
            }

            if((current_time - db_info->last_debounce_time[row][col]) >= DEBOUNCE_TIME_MS) {
                bool changed = false;
                if(measured_state != matrix->debounced_state[row][col])changed=true;
                matrix->debounced_state[row][col] = measured_state;
                for(int coll = 0; coll < NUM_COLS; coll++) {
                    for(int roww = 0; roww < NUM_ROWS; roww++) {
                        if(matrix->debounced_state[roww][coll] == 0){
                            if(changed) {
                                if(roww == 0)keypressed[1] |= 0b00000001;
                                else if(roww == 5)keypressed[1] |= 0b00000001;
                                else if(roww == 1)keypressed[1] |= 0b00000010;
                                else if(roww == 2){
                                    rgbmode++;
                                    if(rgbmode == 3)rgbmode=0;
                                    FlashOptionData(rgbmode,gain_now);
                                }
                                else if(roww == 3){
                                    if(gain_now < 5){
                                        gain_now+=1;
                                        FlashOptionData(rgbmode,gain_now);
                                    }
                                }
                                else if(roww == 4){
                                    if(gain_now > 1){
                                        gain_now-=1;
                                        FlashOptionData(rgbmode,gain_now);
                                    }
                                }
                            }
                        }else{
                            if(changed) {
                                if(roww == 0)keypressed[1] &= 0b11111110;
                                else if(roww == 5 && (!(keypressed[1] & 0b00000001)))keypressed[1] &= 0b11111110;
                                else if(roww == 1)keypressed[1] &= 0b11111101;
                            }                                
                        }

                        }
                    }
                matrix->measuring[row][col] = 0;
            }

        }
    }
    }

volatile uint32_t SysTick_Count;

/*
 * Start up the SysTick IRQ
 */
void systick_init(void)
{
	/* disable default SysTick behavior */
	SysTick->CTLR = 0;
	
	/* enable the SysTick IRQ */
	NVIC_EnableIRQ(SysTicK_IRQn);
	
	/* Set the tick interval to 1ms for normal op */
	SysTick->CMP = (FUNCONF_SYSTEM_CORE_CLOCK/1000)-1;
	
	/* Start at zero */
	SysTick->CNT = 0;
	SysTick_Count = 0;
	
	/* Enable SysTick counter, IRQ, HCLK/1 */
	SysTick->CTLR = SYSTICK_CTLR_STE | SYSTICK_CTLR_STIE |
					SYSTICK_CTLR_STCLK;
}

/*
 * SysTick ISR just counts ticks
 * note - the __attribute__((interrupt)) syntax is crucial!
 */
void SysTick_Handler(void) __attribute__((interrupt));
void SysTick_Handler(void)
{
	// move the compare further ahead in time.
	// as a warning, if more than this length of time
	// passes before triggering, you may miss your
	// interrupt.
	SysTick->CMP += (FUNCONF_SYSTEM_CORE_CLOCK/1000);

	/* clear IRQ */
	SysTick->SR = 0;

	/* update counter */
	SysTick_Count++;
}

// Get Current Time in ms
uint32_t GetSystemTime(void) {
    return SysTick_Count;
}
