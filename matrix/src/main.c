#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#define BAUD_RATE(X) (4.*F_CPU/(X)+.5)
#define U_SEC(X) (F_CPU/1000000*(X)-1)
#define FOR(X) for(uint8_t i=0;i<X;i++)

#define SERCLK 6
#define RCLK 7
#define OE_ 3
#define OE_CMP CMP0BUF

volatile uint8_t disp_r[16]={[0 ... 15]=219};
volatile uint8_t disp_g[16]={[0 ... 15]=109};

static void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}// TCB0
static uint16_t swap(uint8_t i){
	uint8_t r=disp_r[i],g=disp_g[i];
	return~(
	((g>>3)&1)<< 0|((r>>3)&1)<< 1|
	((g>>2)&1)<< 2|((r>>2)&1)<< 3|
	((g>>1)&1)<< 4|((r>>1)&1)<< 5|
	((g>>0)&1)<< 6|((r>>0)&1)<< 7|
	((r>>4)&1)<< 8|((g>>4)&1)<< 9|
	((r>>5)&1)<<10|((g>>5)&1)<<11|
	((r>>6)&1)<<12|((g>>6)&1)<<13|
	((r>>7)&1)<<14|((g>>7)&1)<<15);
}

void main(){
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,0);//CLKCTRL_PDIV_12X_gc|CLKCTRL_PEN_bm);

	// OE# CMP0
	TCA0.SINGLE.CTRLA=TCA_SINGLE_ENABLE_bm;
	TCA0.SINGLE.CTRLB=TCA_SINGLE_CMP0EN_bm|TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	TCA0.SINGLE.PER=255;
	TCA0.SINGLE.OE_CMP=100;

	TCB0.CTRLA=TCB_ENABLE_bm;

	PORTA.DIRSET=_BV(SERCLK)|_BV(RCLK)|_BV(OE_);
	PORTA.OUTSET=_BV(SERCLK)|_BV(RCLK);

	sei();
	while(1)for(uint8_t i=0;i<16;++i){// 1300us
		TCB0.CCMP=U_SEC(20);
		// 4G 4R 3G 3R 2G 2R 1G 1R
		// 5R 5G 6R 6G 7R 7G 8R 8G
		// A1 A2 A3 A4 A5 A6 A7 A8
		// B1 B2 B3 B4 B5 B6 B7 B8 <
		uint32_t w=(1UL<<(i+16))|swap(i);
		for(uint8_t j=0;j<32;++j){// 20*2*32=1280 us
			wait();
			if((w>>j)&1){
				PORTA.OUTCLR=_BV(SERCLK);PORTA.OUTSET=_BV(SERCLK);wait();
			}else{
				PORTA.OUTCLR=_BV(SERCLK);wait();PORTA.OUTSET=_BV(SERCLK);
			}
		}
		TCB0.CCMP=U_SEC(20);
		PORTA.OUTCLR=_BV(RCLK);
		PORTA.OUTSET=_BV(RCLK);
		wait();
	}
}
