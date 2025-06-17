#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#define BAUD_RATE(X) (4.*F_CPU/(X)+.5)
#define U_SEC(X) (F_CPU/1000000*(X)-1)
#define FOR(X) for(uint8_t i=0;i<X;i++)

#define SDA 1
#define SCL 2
#define BTN 3
#define TXD 6
#define RXD 7


volatile uint8_t disp[8]={[0 ... 7]=0xff};

static void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}// TCB0

// UART0 rw
volatile uint8_t t=0;
volatile uint8_t cnt=0;
ISR(USART0_RXC_vect){
	if(1000/500<t)cnt=0;
	t=0;ms=-1;
	uint8_t r=USART0.RXDATAL;
	if(cnt<8)disp[cnt++]=r;
	else if(cnt==8){bri=(bri&0xff00)|r;++cnt;}
	else if(cnt==9){bri=(bri&0x00ff)|(r<<8);++cnt;}
	else{while(!(USART0.STATUS&USART_DREIF_bm));USART0.TXDATAL=r;}
}

void main(){
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,0);//CLKCTRL_PDIV_12X_gc|CLKCTRL_PEN_bm);

	TCB0.CTRLA=TCB_ENABLE_bm;

	// 11500 8N1
	USART0.BAUD=BAUD_RATE(115200);// 9600 too slow !
	USART0.CTRLB=USART_RXEN_bm|USART_TXEN_bm;
	USART0.CTRLA=USART_RXCIE_bm;

	PORTA.DIRSET=_BV(SDA)|_BV(SCL)|_BV(TXD);

	sei();
	while(1)for(uint8_t i=0;i<8;++i){// 1000us
		TCB0.CCMP=U_SEC(10);
		wait();
	}
}
