/*
 * @file    uard_cdc.c
 * @brief   usb cdc to uart demo for CH32V003
 *
 * @author Kanken6174
 * @date 08/05/2025
 *
 */
#include "ch32fun.h"
#include "rv003usb.h"
#include "ch32v003_GPIO_branchless.h"
#include <string.h>

#define UART_BUFFER_SIZE 256
#define UART_BR          115200

volatile char    usb_tx_buffer[UART_BUFFER_SIZE];
volatile int     size_to_send = 0;
volatile int     send_index   = 0;

static uint8_t        rx_buffer[UART_BUFFER_SIZE];
static volatile uint16_t rx_write_pos = 0;
static volatile uint16_t rx_read_pos  = 0;

static uint8_t        tx_buffer[UART_BUFFER_SIZE];
static volatile uint16_t tx_head     = 0;
static volatile uint16_t tx_tail     = 0;
static volatile uint8_t  tx_active   = 0;
static volatile uint16_t tx_last_len = 0;

static void uart_setup(void)
{
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1;
    funPinMode(PD5, GPIO_CFGLR_OUT_10Mhz_AF_PP);
    GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_D, 6), GPIO_pinMode_I_floating, GPIO_Speed_10MHz);
    USART1->CTLR1 = USART_WordLength_8b | USART_Parity_No | USART_Mode_Tx | USART_Mode_Rx;
    USART1->CTLR2 = USART_StopBits_1;
    USART1->CTLR3 = USART_DMAReq_Tx | USART_DMAReq_Rx;
    USART1->BRR   = (FUNCONF_SYSTEM_CORE_CLOCK + UART_BR/2)/UART_BR;
    USART1->CTLR1 |= CTLR1_UE_Set;
}

static void dma_uart_tx_setup(void)
{
    RCC->AHBPCENR |= RCC_AHBPeriph_SRAM | RCC_AHBPeriph_DMA1;
    DMA1_Channel4->CFGR &= ~DMA_CFGR1_EN;
    DMA1_Channel4->PADDR = (uint32_t)&USART1->DATAR;
    DMA1_Channel4->CFGR  = DMA_CFGR1_MINC | DMA_CFGR1_DIR | DMA_CFGR1_TCIE;
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
}

static void dma_uart_rx_setup(void)
{
    RCC->AHBPCENR |= RCC_AHBPeriph_SRAM | RCC_AHBPeriph_DMA1;
    DMA1_Channel5->CFGR &= ~DMA_CFGR1_EN;
    DMA1_Channel5->PADDR = (uint32_t)&USART1->DATAR;
    DMA1_Channel5->MADDR = (uint32_t)rx_buffer;
    DMA1_Channel5->CNTR  = UART_BUFFER_SIZE;
    DMA1_Channel5->CFGR  = DMA_CFGR1_MINC | DMA_CFGR1_CIRC;
    DMA1_Channel5->CFGR |= DMA_CFGR1_EN;
    USART1->CTLR1 |= USART_CTLR1_IDLEIE;
    NVIC_EnableIRQ(USART1_IRQn);
}

static void start_tx_dma(void)
{
    if (tx_active || tx_head == tx_tail) return;
    uint16_t len = (tx_head > tx_tail)
        ? (tx_head - tx_tail)
        : (UART_BUFFER_SIZE - tx_tail);
    tx_last_len = len;
    DMA1_Channel4->CFGR &= ~DMA_CFGR1_EN;
    DMA1_Channel4->CNTR  = len;
    DMA1_Channel4->MADDR = (uint32_t)&tx_buffer[tx_tail];
    DMA1_Channel4->CFGR |= DMA_CFGR1_EN;
    tx_active = 1;
}

__attribute__((interrupt))
void USART1_IRQHandler(void)
{
    if (USART1->STATR & USART_STATR_IDLE) {
        (void)USART1->STATR;
        (void)USART1->DATAR;
        rx_write_pos = (UART_BUFFER_SIZE - DMA1_Channel5->CNTR) % UART_BUFFER_SIZE;
        if (rx_write_pos != rx_read_pos) {
            uint16_t len;
            uint8_t *src = &rx_buffer[rx_read_pos];
            if (rx_write_pos > rx_read_pos) {
                len = rx_write_pos - rx_read_pos;
                memcpy((void*)usb_tx_buffer, src, len);
            } else {
                uint16_t first = UART_BUFFER_SIZE - rx_read_pos;
                memcpy((void*)usb_tx_buffer, src, first);
                memcpy((void*)(usb_tx_buffer + first), rx_buffer, rx_write_pos);
                len = first + rx_write_pos;
            }
            rx_read_pos   = rx_write_pos;
            size_to_send  = len;
            send_index    = 0;
        }
    }
}

__attribute__((interrupt))
void DMA1_Channel4_IRQHandler(void)
{
    DMA1->INTFCR |= DMA_CTCIF4;
    tx_tail    = (tx_tail + tx_last_len) % UART_BUFFER_SIZE;
    tx_active  = 0;
    start_tx_dma();
}

void usb_handle_user_data(struct usb_endpoint *e, int ep, uint8_t *data, int len, struct rv003usb_internal *ist)
{
    if (ep != 2 || len == 0) return;
    for (int i = 0; i < len; i++)
        tx_buffer[tx_head = (tx_head + 1) % UART_BUFFER_SIZE] = data[i];
    start_tx_dma();
}

void usb_handle_user_in_request(struct usb_endpoint *e, uint8_t *scratchpad, int ep, uint32_t tok, struct rv003usb_internal *ist)
{
    if (ep == 3) {
        if (send_index < size_to_send) {
            int chunk = (size_to_send - send_index > 8) ? 8 : size_to_send - send_index;
            usb_send_data((uint8_t*)(usb_tx_buffer + send_index), chunk, 0, tok);
            send_index += chunk;
        } else {
            usb_send_empty(tok);
        }
    } else {
        usb_send_empty(tok);
    }
}

void usb_handle_other_control_message(struct usb_endpoint *e, struct usb_urb *s, struct rv003usb_internal *ist)
{
    e->opaque = 0;
}

int main(void)
{
    SystemInit();
    uart_setup();
    dma_uart_tx_setup();
    dma_uart_rx_setup();
    usb_setup();
    while (1) {
        uint32_t *ue = GetUEvent();
        (void)ue;
    }
}
