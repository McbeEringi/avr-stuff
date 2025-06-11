#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#define BAUD_RATE(X) (4.*F_CPU/(X)+.5)
#define U_SEC(X) (F_CPU/1000000*(X)-1)
#define FOR(X) for(uint8_t i=0;i<X;i++)

#if defined(REV1)
#define LSBFIRST
#define SERCLK 1
#define RCLK 2
#define OE_ 3
#define TXD 6
#define RXD 7
#elif defined(REV2)
#define MSBFIRST
#define UART_ALT
#define SERCLK 6
#define RCLK 7
#define OE_ 3
#define TXD 1
#define RXD 2
#else
#error "REV?"
#endif

volatile uint8_t disp[8]={// -HELLO!-
	2,
	0b01101110,
	0b10011110,
	0b00011100,
	0b00011100,
	0b11111100,
	0b01000001,
	2
};
volatile uint16_t bri=0xaaaa;

static void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}// TCB0

// UART0 rw
volatile uint8_t t=0;
volatile uint8_t cnt=0;
ISR(USART0_RXC_vect){
	if(1000/500<t)cnt=0;
	t=0;
	uint8_t r=USART0.RXDATAL;
	if(cnt<8)disp[cnt++]=r;
	else{
		while(!(USART0.STATUS&USART_DREIF_bm));USART0.TXDATAL=r;
	}
}

void main(){
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,0);//CLKCTRL_PDIV_12X_gc|CLKCTRL_PEN_bm);

	// OE# CMP0
	TCA0.SINGLE.CTRLA=TCA_SINGLE_ENABLE_bm;
	TCA0.SINGLE.CTRLB=TCA_SINGLE_CMP0EN_bm|TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	TCA0.SINGLE.PER=255;

	TCB0.CTRLA=TCB_ENABLE_bm;

	// 11500 8N1
#ifdef UART_ALT
	PORTMUX.CTRLB=PORTMUX_USART0_ALTERNATE_gc;
#endif
	USART0.BAUD=BAUD_RATE(115200);// 9600 too slow !
	USART0.CTRLB=USART_RXEN_bm|USART_TXEN_bm;
	USART0.CTRLA=USART_RXCIE_bm;

	PORTA.DIRSET=_BV(SERCLK)|_BV(RCLK)|_BV(OE_)|_BV(TXD);
	PORTA.OUTSET=_BV(SERCLK)|_BV(RCLK);

	sei();
	while(1)for(uint8_t i=0;i<8;++i){// 500us
		TCB0.CCMP=U_SEC(10);
		// >> ABCDEFGd 01234567
		uint16_t w=(disp[i]<<8)|((1<<(7-i))^255);
		for(uint8_t
#if defined(LSBFIRST)
			j=0;j<16;++j
#elif defined(MSBFIRST)
			j=15;j<16;--j
#else
#error "bit order?"
#endif
			){// 10*3*16=480 us
			wait();
			PORTA.OUTCLR=_BV(SERCLK);
			if((w>>j)&1){PORTA.OUTSET=_BV(SERCLK);wait();}
			else{wait();PORTA.OUTSET=_BV(SERCLK);}
			wait();
		}
		TCB0.CCMP=U_SEC(520);
		PORTA.OUTCLR=_BV(RCLK);
		PORTA.OUTSET=_BV(RCLK);
		TCA0.SINGLE.CMP0BUF=2<<((4-((bri>>(i*2))&0b11))*2)-1;
		wait();if(~t)++t;
	}
}
