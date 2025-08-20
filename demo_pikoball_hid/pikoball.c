// ─────────────────────────────────────────────────────────────────────────────
//  Trackball firmware (CH32)
//  Notes:
//    - Keep USB ISR light; all sampling happens in main loop
//    - Per-roller delta is rectified; direction comes from opposite-pair diff
// ─────────────────────────────────────────────────────────────────────────────

#include "ch32fun.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "help_functions.h"
#include "rv003usb.h"

// ./minichlink -D to use NRST; also PD1 needs pull-up
// PC0, PC7  -> buttons
// PD2, PA1, PA2, PC4 -> analog rollers

// ─────────────────────────────────  CONFIG  ──────────────────────────────────
#define ADC_DEAD         10    // LSB dead-zone before ± movement (USB handler)
#define ADC_NOISE        1     // ignore deltas ≤ this

// ADC channel → sensor mapping
#define ADC_N  GPIO_Ain3_D2   // North roller
#define ADC_E  GPIO_Ain1_A1   // East  roller
#define ADC_S  GPIO_Ain2_C4   // South roller
#define ADC_W  GPIO_Ain0_A2   // West  roller

// ─────────────────────────────  roller struct  ───────────────────────────────
struct Roller {
    int16_t previous;  // last sample
    int32_t acc;       // accumulated rectified delta
};

static struct Roller rN, rE, rS, rW;

// ──────────────────────────────  ADC helpers  ────────────────────────────────
static inline int16_t read_avg_adc(int16_t channel)
{
    int32_t sum = 0;
    for (uint8_t i = 0; i < 16; i++) {
        sum += GPIO_analogRead(channel);
    }
    return (int16_t)(sum / 16);
}

static inline int32_t scale_gain_s10(int32_t v, int16_t gain10)
{
    return (v >= 0) ? (v * gain10 + 5) / 10
                    : (v * gain10 - 5) / 10;  // round toward zero
}

static void averager_sample(struct Roller *r, int16_t channel)
{
    int16_t s = read_avg_adc(channel);
    int16_t d = s - r->previous;  // high-pass
    r->previous = s;

    // rectified magnitude (direction from opposite-pair comparison)
    int16_t a = (d >= 0) ? d : -d;
    if (a <= ADC_NOISE) return;

    r->acc += scale_gain_s10(a, (int16_t)gain_now);
}

// ─────────────────────────────────── main ────────────────────────────────────
int main(void)
{
    SystemInit();
    Delay_Ms(1);       // ensure USB re-enumeration after boot/reset (>2.5 µs)
    systick_init();
    GPIO_Init_All();

    ButtonMatrix_Init(&btn_matrix);
    Debounce_Init(&debounce_info);
    usb_setup();

    WS2812BDMAInit();
    frameno = 0;
    for (uint8_t k = 0; k < NR_LEDS; k++) {
        phases[k] = (uint16_t)k << 8;
    }
    int tweendir = 0;

    rgbmode  = OB->Data0;
    gain_now = OB->Data1;
    if (gain_now == 0) {
        gain_now = 2;
        FlashOptionData(rgbmode, gain_now);
    }

    while (1) {
#if RV003USB_EVENT_DEBUGGING
        uint32_t *ue = GetUEvent();
        if (ue) {
            printf("%lu %lx %lx %lx\n", ue[0], ue[1], ue[2], ue[3]);
        }
#endif
        uint32_t current_time = GetSystemTime();

        averager_sample(&rN, ADC_N);
        averager_sample(&rE, ADC_E);
        averager_sample(&rS, ADC_S);
        averager_sample(&rW, ADC_W);

        ButtonMatrix_Scan_Debounced(&btn_matrix, &debounce_info, current_time);

        while (WS2812BLEDInUse)
            ;
        frameno++;
        if (frameno == 1024) {
            tweendir = 1;
        }
        if (frameno == 2048) {
            tweendir = -1;
            frameno = 0;
        }
        if (tweendir) {
            int t = tween + tweendir * 4;
            if (t > 255)      t = 255;
            if (t < -NR_LEDS) t = -NR_LEDS;
            tween = t;
        }
        for (uint8_t k = 0; k < NR_LEDS; k++) {
            phases[k] += 3 * (rands[k] + 0x0F);
        }
        WS2812BDMAStart(NR_LEDS);
        Delay_Ms(1);
    }
}

// ────────────────────────────────  USB ISR  ──────────────────────────────────
uint8_t tsajoystick[8] = {0};

void usb_handle_user_in_request(struct usb_endpoint *e,
                                uint8_t *scratchpad,
                                int endp,
                                uint32_t sendtok,
                                struct rv003usb_internal *ist)
{
    if (endp == 1) {
        int16_t dx = rE.acc - rW.acc;
        int16_t dy = rS.acc - rN.acc;

        rE.acc = rW.acc = rN.acc = rS.acc = 0;  // reset for next frame

        if (dx > 127)  dx = 127;
        if (dx < -127) dx = -127;
        if (dy > 127)  dy = 127;
        if (dy < -127) dy = -127;

        tsajoystick[0] = keypressed[1];
        tsajoystick[1] = (uint8_t)dx;  // pack as raw byte (kept as-is)
        tsajoystick[2] = (uint8_t)dy;

        usb_send_data(tsajoystick, 4, 0, sendtok);

        for (uint8_t k = 0; k < 8; k++) {
            tsajoystick[k] = 0;
        }
    } else {
        usb_send_empty(sendtok);
    }
}
