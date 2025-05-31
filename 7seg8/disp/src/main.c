#include <avr/io.h>
#include <avr/delay.h>

#define SERCLK 1
#define RCLK 2
#define OE_ 3
#define TXD 6
#define RXD 7

const uint8_t num[]={
	0b11111100,0b01100000,0b11011010,0b11110010,// 0123
	0b01100110,0b10110110,0b10111110,0b11100000,// 4567
	0b11111110,0b11110110,0b11101110,0b00111110,// 89Ab
	0b10011100,0b01111010,0b10011110,0b10001110 // CdEf
};

uint8_t disp[8]={// -HELLO!-
	2,
	0b01101110,
	0b10011110,
	0b00011100,
	0b00011100,
	0b11111100,
	0b01000001,
	2
};


static void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}// TCB0

void main(){
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,0);//CLKCTRL_PDIV_12X_gc|CLKCTRL_PEN_bm);

	// OE# CMP0
	TCA0.SINGLE.CTRLA=TCA_SINGLE_ENABLE_bm;
	TCA0.SINGLE.CTRLB=TCA_SINGLE_CMP0EN_bm|TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	TCA0.SINGLE.PER=255;
	TCA0.SINGLE.CMP0BUF=0;

	TCB0.CTRLA=TCB_ENABLE_bm;
	TCB0.CCMP=200-1;// 10us = 100kHz = 20000k/200

	// 9600 8N1
	USART0.BAUD=1302;//4*F_CPU/9600;
	USART0.CTRLC=USART_CHSIZE_8BIT_gc;
	USART0.CTRLB=USART_RXEN_bm|USART_TXEN_bm;

	PORTA.DIRSET=_BV(SERCLK)|_BV(RCLK)|_BV(OE_)|_BV(TXD);
	PORTA.OUTSET=_BV(SERCLK)|_BV(RCLK);

	// for(uint8_t i=0;i<8;i++)disp[i]=num[i+1]|(i&1);
	uint8_t cnt=0;
	uint16_t t=0;

	while(1)for(uint8_t i=0;i<8;++i){// 500us
		// >> ABCDEFGd 01234567
		uint16_t w=(disp[i]<<8)|((1<<(7-i))^255);
		for(uint8_t j=0;j<16;++j){// 10*3*16=480 us
			wait();
			PORTA.OUTCLR=_BV(SERCLK);
			if(((w>>j)&1)){PORTA.OUTSET=_BV(SERCLK);wait();}
			else{wait();PORTA.OUTSET=_BV(SERCLK);}
			wait();
		}
		PORTA.OUTCLR=_BV(RCLK);
		PORTA.OUTSET=_BV(RCLK);
		// if(USART0.STATUS&USART_RXCIF_bm){
		// 	++cnt;
		// 	while (!(USART0.STATUS & USART_DREIF_bm));
			USART0.TXDATAL = 97; // USART0.RXDATAL;
		// }
		disp[0]=num[cnt&15]|(~(t/1000)&1);//num[cnt&15];
		wait();wait();++t;
	}
}
