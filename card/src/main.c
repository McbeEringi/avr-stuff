#include <avr/io.h>

#define NUM_LED 30
#define NUM_COL 6
#define NUM_ROW 5
#define NUM_W 10
#define NUM_H 7 
#define FPS 100

uint8_t w[NUM_LED]={0};
// r1c1
// r1c2
// r1c3
// r1c4
// r1c5
// r2c1
// r2c2
// ...

const uint8_t i2p[]={
	0x05,0x59,0x65,0x64,0x50,0x04,
	0x06,0x49,0x66,0x63,0x40,0x03,
	0x07,0x39,0x67,0x62,0x30,0x02,
	0x08,0x29,0x68,0x61,0x20,0x01,
	0x09,0x19,0x69,0x60,0x10,0x00
};
const uint8_t i2t[]={
	0,9,14,15,20,29,
	1,8,13,16,21,28,
	2,7,12,17,22,27,
	3,6,11,18,23,26,
	4,5,10,19,24,25
};

static void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}


static void row(uint8_t i){
	// set_row
	uint8_t x=~(1<<i);
	VPORTA.OUT=
		(((x>>0)&1)<<2)|
		(((x>>1)&1)<<1)|
		(((x>>2)&1)<<6)|
		(((x>>3)&1)<<7);
	VPORTB.OUT=
		(((x>>4)&1)<<3);

	// flush_column
	i*=NUM_COL;
	TCA0.SPLIT.LCMP0=w[i];
	TCA0.SPLIT.LCMP1=w[++i];
	TCA0.SPLIT.LCMP2=w[++i];
	TCA0.SPLIT.HCMP0=w[++i];
	TCA0.SPLIT.HCMP1=w[++i];
	TCA0.SPLIT.HCMP2=w[++i];
	TCA0.SPLIT.LCNT=
	TCA0.SPLIT.HCNT=0;
}

int main() {
	_PROTECTED_WRITE(CLKCTRL_MCLKCTRLB,CLKCTRL_PDIV_8X_gc|CLKCTRL_PEN_bm);
	PORTA.DIRSET=0xfe;
	PORTB.DIRSET=0x0f;

	TCA0.SPLIT.CTRLD=
		TCA_SPLIT_SPLITM_bm;
	TCA0.SPLIT.CTRLB=
		TCA_SPLIT_HCMP2EN_bm|
		TCA_SPLIT_HCMP1EN_bm|
		TCA_SPLIT_HCMP0EN_bm|
		TCA_SPLIT_LCMP2EN_bm|
		TCA_SPLIT_LCMP1EN_bm|
		TCA_SPLIT_LCMP0EN_bm;
	TCA0.SPLIT.CTRLA=
		TCA_SPLIT_CLKSEL_DIV1_gc|
		TCA_SPLIT_ENABLE_bm;

	TCB0.CTRLA=
		TCB_CLKSEL_CLKDIV2_gc|
		TCB_ENABLE_bm;
	TCB0.CCMP=((F_CPU/8/2)/FPS)/(NUM_ROW+2)-1;


	uint16_t t=0;
	uint8_t mode=0;
	while(1){
		row(NUM_ROW+1);
		for(uint8_t i=0;i<NUM_LED;++i){
			switch(mode){
				case 0:{
					w[i]=(i2t[i]+NUM_LED-t*NUM_LED/100-1)%NUM_LED*2;
					break;
				}
				case 1:{
					w[i]=((i2p[i]&0x0f)+NUM_LED-t*NUM_W/100-1)%NUM_LED*2;
					break;
				}
				case 2:{
					w[i]=((i2p[i]>>4)+NUM_LED-t*NUM_H/100-1)%NUM_LED*2;
					break;
				}
			}
		}
		++t;
		while(100<=t){t-=100;mode=++mode%3;}
		wait();
		for(uint8_t i=0;i<NUM_ROW;++i){
			row(i);
			wait();
		}
	}
}
