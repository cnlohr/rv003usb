#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"
#include "ch32v003_GPIO_branchless.h"

// Define Output Pins
#define GPIO_OUT_PC7  GPIOv_from_PORT_PIN(GPIO_port_C, 7)
#define GPIO_OUT_PC6  GPIOv_from_PORT_PIN(GPIO_port_C, 6)
#define GPIO_OUT_PC5  GPIOv_from_PORT_PIN(GPIO_port_C, 5)
#define GPIO_OUT_PC4  GPIOv_from_PORT_PIN(GPIO_port_C, 4)
#define GPIO_OUT_PD7  GPIOv_from_PORT_PIN(GPIO_port_D, 7)
#define GPIO_OUT_PC2  GPIOv_from_PORT_PIN(GPIO_port_C, 2)
#define GPIO_OUT_PC1  GPIOv_from_PORT_PIN(GPIO_port_C, 1)
#define GPIO_OUT_PD1  GPIOv_from_PORT_PIN(GPIO_port_D, 1)

// Define Input Pins
#define GPIO_IN_PD0   GPIOv_from_PORT_PIN(GPIO_port_D, 0)
#define GPIO_IN_PD2   GPIOv_from_PORT_PIN(GPIO_port_D, 2)
#define GPIO_IN_PD5   GPIOv_from_PORT_PIN(GPIO_port_D, 5)
#define GPIO_IN_PD6   GPIOv_from_PORT_PIN(GPIO_port_D, 6)
#define GPIO_IN_PC3   GPIOv_from_PORT_PIN(GPIO_port_C, 3)
#define GPIO_IN_PA1   GPIOv_from_PORT_PIN(GPIO_port_A, 1)
#define GPIO_IN_PA2   GPIOv_from_PORT_PIN(GPIO_port_A, 2)
#define GPIO_IN_PC0   GPIOv_from_PORT_PIN(GPIO_port_C, 0)
// Define Number of Rows and Columns
#define NUM_COLS 8
#define NUM_ROWS 8

// Define Columns (Outputs)
const int columns_ports[NUM_COLS] = {
    GPIO_port_C, // Column 0 - PC7
    GPIO_port_C, // Column 1 - PC6
    GPIO_port_C, // Column 2 - PC5
    GPIO_port_C, // Column 3 - PC4
    GPIO_port_D, // Column 4 - PD7
    GPIO_port_C, // Column 5 - PC2
    GPIO_port_C, // Column 6 - PC1
    GPIO_port_D  // Column 7 - PD1
};

const uint8_t columns_pins[NUM_COLS] = {
    7, // PC7
    6, // PC6
    5, // PC5
    4, // PC4
    7, // PD7
    2, // PC2
    1, // PC1
    0  // PD1
};

// Define Rows (Inputs)
const int rows_ports[NUM_ROWS] = {
    GPIO_port_D, // Row 0 - PD0
    GPIO_port_D, // Row 1 - PD2
    GPIO_port_D, // Row 2 - PD5
    GPIO_port_D, // Row 3 - PD6
    GPIO_port_C, // Row 4 - PC3
    GPIO_port_A, // Row 5 - PA1
    GPIO_port_A, // Row 6 - PA2
    GPIO_port_C,  // Row 7 - PC0
};

const uint8_t rows_pins[NUM_ROWS] = {
    1, // PD0
    2, // PD2
    5, // PD5
    6, // PD6
    3, // PC3
    1, // PA1
    2, // PA2
    0,  // PC0
    };

// Define HID Usage IDs (Partial List)
#define HID_KEY_A          0x04
#define HID_KEY_B          0x05
#define HID_KEY_C          0x06
#define HID_KEY_D          0x07
#define HID_KEY_E          0x08
#define HID_KEY_F          0x09
#define HID_KEY_G          0x0A
#define HID_KEY_H          0x0B
#define HID_KEY_I          0x0C
#define HID_KEY_J          0x0D
#define HID_KEY_K          0x0E
#define HID_KEY_L          0x0F
#define HID_KEY_M          0x10
#define HID_KEY_N          0x11
#define HID_KEY_O          0x12
#define HID_KEY_P          0x13
#define HID_KEY_Q          0x14
#define HID_KEY_R          0x15
#define HID_KEY_S          0x16
#define HID_KEY_T          0x17
#define HID_KEY_U          0x18
#define HID_KEY_V          0x19
#define HID_KEY_W          0x1A
#define HID_KEY_X          0x1B
#define HID_KEY_Y          0x1C
#define HID_KEY_Z          0x1D
#define HID_KEY_1          0x1E
#define HID_KEY_2          0x1F
#define HID_KEY_3          0x20
#define HID_KEY_4          0x21
#define HID_KEY_5          0x22
#define HID_KEY_6          0x23
#define HID_KEY_7          0x24
#define HID_KEY_8          0x25
#define HID_KEY_9          0x26
#define HID_KEY_0          0x27
#define HID_KEY_ENTER      0x28
#define HID_KEY_ESC        0x29
#define HID_KEY_BACKSPACE  0x2A
#define HID_KEY_TAB        0x2B
#define HID_KEY_SPACE      0x2C
#define HID_KEY_MINUS      0x2D
#define HID_KEY_EQUAL      0x2E
#define HID_KEY_LEFTBRACE  0x2F
#define HID_KEY_RIGHTBRACE 0x30
#define HID_KEY_BACKSLASH  0x31
#define HID_KEY_SEMICOLON  0x33
#define HID_KEY_QUOTE      0x34
#define HID_KEY_GRAVE      0x35
#define HID_KEY_COMMA      0x36
#define HID_KEY_DOT        0x37
#define HID_KEY_SLASH      0x38
#define HID_KEY_CAPSLOCK   0x39
#define HID_KEY_F1         0x3A
#define HID_KEY_F2         0x3B
#define HID_KEY_F3         0x3C
#define HID_KEY_F4         0x3D
#define HID_KEY_F5         0x3E
#define HID_KEY_F6         0x3F
#define HID_KEY_F7         0x40
#define HID_KEY_F8         0x41
#define HID_KEY_F9         0x42
#define HID_KEY_F10        0x43
#define HID_KEY_F11        0x44
#define HID_KEY_F12        0x45

#define HID_KEY_RIGHT      0x4f
#define HID_KEY_LEFT       0x50
#define HID_KEY_DOWN       0x51
#define HID_KEY_UP         0x52

#define HID_KEY_LEFT_SHIFT 0xE1
#define HID_KEY_RIGHT_SHIFT 0xE5
#define HID_KEY_LEFT_CTRL  0xE0
#define HID_KEY_RIGHT_CTRL 0xE4
#define HID_KEY_LEFT_WIN   0xE3
#define HID_KEY_LEFT_ALT   0xE2
#define HID_KEY_RIGHT_ALT  0xE6
#define HID_KEY_DEL        0x4C
#define HID_KEY_PTRSC      0x46

#define HID_KEY_FN         0xFC
#define HID_KEY_INSERT     0x49
#define HID_KEY_HOME       0x4A

const uint8_t keymap[NUM_ROWS][NUM_COLS] = {
    { HID_KEY_6, HID_KEY_5, HID_KEY_4, HID_KEY_3, HID_KEY_2, HID_KEY_1, HID_KEY_GRAVE, HID_KEY_ESC },
    { HID_KEY_7, HID_KEY_8, HID_KEY_9, HID_KEY_0, HID_KEY_MINUS, HID_KEY_EQUAL, HID_KEY_BACKSPACE, HID_KEY_ENTER },
    { HID_KEY_LEFTBRACE, HID_KEY_RIGHTBRACE, HID_KEY_P, HID_KEY_O, HID_KEY_I, HID_KEY_U, HID_KEY_Y, HID_KEY_T },
    { HID_KEY_CAPSLOCK, HID_KEY_A, HID_KEY_S, HID_KEY_TAB, HID_KEY_Q, HID_KEY_W, HID_KEY_E, HID_KEY_R },
    { HID_KEY_SEMICOLON, HID_KEY_L, HID_KEY_K, HID_KEY_J, HID_KEY_H, HID_KEY_G, HID_KEY_F, HID_KEY_D },
    { HID_KEY_N, HID_KEY_M, HID_KEY_COMMA, HID_KEY_DOT, HID_KEY_SLASH, HID_KEY_RIGHT_SHIFT, HID_KEY_BACKSLASH, HID_KEY_QUOTE },
    { HID_KEY_LEFT_CTRL, HID_KEY_LEFT_SHIFT, HID_KEY_SLASH, HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_V, HID_KEY_B },
    { HID_KEY_FN, HID_KEY_LEFT_WIN, HID_KEY_LEFT_ALT, HID_KEY_RIGHT_ALT, HID_KEY_RIGHT_CTRL, HID_KEY_DEL, HID_KEY_PTRSC, HID_KEY_SPACE }
};
const uint8_t keymap2[NUM_ROWS][NUM_COLS] = {
    { HID_KEY_F6, HID_KEY_F5, HID_KEY_F4, HID_KEY_F3, HID_KEY_F2, HID_KEY_F1, HID_KEY_GRAVE, HID_KEY_ESC },
    { HID_KEY_F7, HID_KEY_F8, HID_KEY_F9, HID_KEY_F10, HID_KEY_F11, HID_KEY_F12, HID_KEY_BACKSPACE, HID_KEY_ENTER },
    { HID_KEY_LEFTBRACE, HID_KEY_RIGHTBRACE, HID_KEY_P, HID_KEY_O, HID_KEY_I, HID_KEY_U, HID_KEY_Y, HID_KEY_T },
    { HID_KEY_CAPSLOCK, HID_KEY_LEFT, HID_KEY_DOWN, HID_KEY_TAB, HID_KEY_Q, HID_KEY_UP, HID_KEY_E, HID_KEY_R },
    { HID_KEY_SEMICOLON, HID_KEY_L, HID_KEY_K, HID_KEY_J, HID_KEY_H, HID_KEY_G, HID_KEY_F, HID_KEY_RIGHT },
    { HID_KEY_N, HID_KEY_M, HID_KEY_COMMA, HID_KEY_DOT, HID_KEY_SLASH, HID_KEY_RIGHT_SHIFT, HID_KEY_BACKSLASH, HID_KEY_QUOTE },
    { HID_KEY_LEFT_CTRL, HID_KEY_LEFT_SHIFT, HID_KEY_SLASH, HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_V, HID_KEY_B },
    { HID_KEY_FN, HID_KEY_LEFT_WIN, HID_KEY_LEFT_ALT, HID_KEY_RIGHT_ALT, HID_KEY_RIGHT_CTRL, HID_KEY_INSERT, HID_KEY_HOME, HID_KEY_SPACE }
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

    for(int col = 0; col < NUM_COLS; col++) {
        GPIO_pinMode(GPIOv_from_PORT_PIN(columns_ports[col], columns_pins[col]), GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
    }

    for(int row = 0; row < NUM_ROWS; row++) {
        GPIO_pinMode(GPIOv_from_PORT_PIN(rows_ports[row], rows_pins[row]), GPIO_pinMode_I_pullDown, GPIO_Speed_In);
    }

    for(int col = 0; col < NUM_COLS; col++) {
        GPIO_digitalWrite(GPIOv_from_PORT_PIN(columns_ports[col], columns_pins[col]), low);
    }
}

// Initialize Button Matrix State
void ButtonMatrix_Init(ButtonMatrix* matrix) {
    for(int row = 0; row < NUM_ROWS; row++) {
        for(int col = 0; col < NUM_COLS; col++) {
            matrix->measuring[row][col] = 0;
            matrix->debounced_state[row][col] = 0;
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
        // Activate the current column (active low)
        GPIO_digitalWrite(GPIOv_from_PORT_PIN(columns_ports[col], columns_pins[col]), high); // high

        // Small delay to allow signals to settle
        Delay_Ms(1);
        // Read all rows
        for(int row = 0; row < NUM_ROWS; row++) {
            uint8_t measured_state = GPIO_digitalRead(GPIOv_from_PORT_PIN(rows_ports[row], rows_pins[row])); // Active high
            if(measured_state != matrix->debounced_state[row][col] && matrix->measuring[row][col] == 0) {
                db_info->last_debounce_time[row][col] = current_time;
                matrix->measuring[row][col] = 1;
            }
            // printf("%lu,%d,%d ",current_time - db_info->last_debounce_time[row][col],measured_state,matrix->debounced_state[row][col]);

            if((current_time - db_info->last_debounce_time[row][col]) >= DEBOUNCE_TIME_MS) {
                if(measured_state != matrix->debounced_state[row][col]) {
                    matrix->debounced_state[row][col] = measured_state;
                    if(measured_state){
                        if(keypressed[0] == 0){
                            uint8_t kpc=0;
                            for(int coll = 0; coll < NUM_COLS; coll++) {
                                for(int roww = 0; roww < NUM_ROWS; roww++) {
                                    if(matrix->debounced_state[roww][coll]){
                                        if(keymap[roww][coll] == HID_KEY_LEFT_CTRL)keypressed[1] |= 0b00000001;
                                        else if(keymap[roww][coll] == HID_KEY_LEFT_SHIFT)keypressed[1] |= 0b00000010;
                                        else if(keymap[roww][coll] == HID_KEY_LEFT_ALT)keypressed[1] |= 0b00000100;
                                        else if(keymap[roww][coll] == HID_KEY_LEFT_WIN)keypressed[1] |= 0b00001000;
                                        else if(keymap[roww][coll] == HID_KEY_RIGHT_CTRL)keypressed[1] |= 0b00010000;
                                        else if(keymap[roww][coll] == HID_KEY_RIGHT_SHIFT)keypressed[1] |= 0b00100000;
                                        else if(keymap[roww][coll] == HID_KEY_RIGHT_ALT)keypressed[1] |= 0b01000000;
                                        else if(kpc<6){
                                            if(matrix->debounced_state[7][0] == 1){
                                                keypressed[2+kpc] = keymap2[roww][coll];                                            
                                            }else{
                                                keypressed[2+kpc] = keymap[roww][coll];
                                            }
                                            kpc++;
                                        }
                                    }
                                }
                            }
                            keypressed[0] = 1;
                        }
                    }
                }             
                matrix->measuring[row][col] = 0;
            }

        }
        // printf("\n\r");
        // Deactivate the current column
        GPIO_digitalWrite(GPIOv_from_PORT_PIN(columns_ports[col], columns_pins[col]), low); // High
    }
    // printf("\n\r");
    // printf("pressed %d\n\r",keypressed);
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
