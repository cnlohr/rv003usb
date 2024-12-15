#ifndef SWIOH
#define SWIOH
// #include <ch32v003fun.h>
#define T1COEFF 2

#define DMCONTROL      0x10
#define DMSTATUS       0x11
#define DMHARTINFO     0x12
#define DMABSTRACTCS   0x16
#define DMCOMMAND      0x17
#define DMABSTRACTAUTO 0x18
#define DMPROGBUF0     0x20
#define DMPROGBUF1     0x21
#define DMPROGBUF2     0x22
#define DMPROGBUF3     0x23
#define DMPROGBUF4     0x24
#define DMPROGBUF5     0x25
#define DMPROGBUF6     0x26
#define DMPROGBUF7     0x27

#define DMCPBR       0x7C
#define DMCFGR       0x7D
#define DMSHDWCFGR   0x7E

static inline void PrecDelay( int delay )
{
	asm volatile(
"1:	addi %[delay], %[delay], -1\n"
"bne %[delay], x0, 1b\n" :[delay]"+r"(delay)  );
}

static inline void Send1Bit(uint8_t t1coeff)
{
	// Low for a nominal period of time.
	// High for a nominal period of time.

	AFIO->PCFR1 |= AFIO_PCFR1_SWJ_CFG_DISABLE;
	funDigitalWrite(PD1, 0);
  AFIO->PCFR1 &= ~(AFIO_PCFR1_SWJ_CFG_DISABLE);
  PrecDelay(t1coeff);
  
	AFIO->PCFR1 |= AFIO_PCFR1_SWJ_CFG_DISABLE;
	funDigitalWrite(PD1, 1);
  AFIO->PCFR1 &= ~(AFIO_PCFR1_SWJ_CFG_DISABLE);
	PrecDelay(t1coeff);
}

static inline void Send0Bit(uint8_t t1coeff)
{
	// Low for a LONG period of time.
	// High for a nominal period of time.
	AFIO->PCFR1 |= AFIO_PCFR1_SWJ_CFG_DISABLE;
	funDigitalWrite(PD1, 0);
  AFIO->PCFR1 &= ~(AFIO_PCFR1_SWJ_CFG_DISABLE);
	PrecDelay(t1coeff*4);
	AFIO->PCFR1 |= AFIO_PCFR1_SWJ_CFG_DISABLE;
	funDigitalWrite(PD1, 1);
  AFIO->PCFR1 &= ~(AFIO_PCFR1_SWJ_CFG_DISABLE);
	PrecDelay(t1coeff);
}

static void MCFWriteReg32(uint8_t command, uint32_t value, uint8_t t1coeff)
{

 	funDigitalWrite(PD1, 1);
	// funPinMode(PD1, GPIO_Speed_50MHz | GPIO_CNF_OUT_PP);

	__disable_irq();
	Send1Bit(t1coeff);
	uint32_t mask;
	for( mask = 1<<6; mask; mask >>= 1 )
	{
		if( command & mask )
			Send1Bit(t1coeff);
		else
			Send0Bit(t1coeff);
	}
	Send1Bit(t1coeff);
	for( mask = 1<<31; mask; mask >>= 1 )
	{
		if( value & mask )
			Send1Bit(t1coeff);
		else
			Send0Bit(t1coeff);
	}
  __enable_irq();
	Delay_Ms(8); // Sometimes 2 is too short.
}

void attempt_unlock(uint8_t t1coeff) {
  if (*DMSTATUS_SENTINEL) {
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;
    AFIO->PCFR1 &= ~(AFIO_PCFR1_SWJ_CFG);
    funPinMode(PD1, GPIO_Speed_50MHz | GPIO_CNF_OUT_PP);
    MCFWriteReg32(DMSHDWCFGR, 0x5aa50000 | (1<<10), t1coeff);
    MCFWriteReg32(DMCFGR, 0x5aa50000 | (1<<10), t1coeff);
    MCFWriteReg32(DMCFGR, 0x5aa50000 | (1<<10), t1coeff);
    MCFWriteReg32(DMABSTRACTAUTO, 0x00000000, t1coeff);
    MCFWriteReg32(DMCONTROL, 0x80000001 | (1<<10), t1coeff);
    MCFWriteReg32(DMCONTROL, 0x40000001 | (1<<10), t1coeff);
  }
}
#endif