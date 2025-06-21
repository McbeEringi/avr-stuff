#include <avr/io.h>
#include <avr/interrupt.h>
#define BAUD_RATE(X) (4.*F_CPU/(X)+.5)
#define U_SEC(X) (F_CPU/1000000*(X)-1)
#define FOR(X) for(uint8_t i=0;i<X;i++)

#define SERCLK 6
#define RCLK 7
#define OE_ 3
#define OE_CMP CMP0BUF
#define ADDR 0x12

volatile uint16_t disp_r[8]={[0 ... 7]=0};
volatile uint16_t disp_g[8]={
	// [0 ... 7]=0xffff
	// 0b0100110011001100,
	// 0b1010101010101010,
	// 0b1110101010101100,
	// 0b1010110011001010,
	0b1010111000000000,
	0b1010100000000000,
	0b1010100000000000,
	0b1010111011101110,
	0b0000000010101010,
	0b1110101011101110,
	0b1010010010101010,
	0b1110101011101110,
};

static void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}// TCB0
static uint16_t swap(uint8_t i){
	uint8_t r=disp_r[i&7]>>(~i&8),g=disp_g[i&7]>>(~i&8);
	return~(
	((g>>4)&1)<< 0|((r>>4)&1)<< 1|
	((g>>5)&1)<< 2|((r>>5)&1)<< 3|
	((g>>6)&1)<< 4|((r>>6)&1)<< 5|
	((g>>7)&1)<< 6|((r>>7)&1)<< 7|
	((r>>3)&1)<< 8|((g>>3)&1)<< 9|
	((r>>2)&1)<<10|((g>>2)&1)<<11|
	((r>>1)&1)<<12|((g>>1)&1)<<13|
	((r>>0)&1)<<14|((g>>0)&1)<<15);
}

volatile cnt=0;
ISR(TWI0_TWIS_vect) {
	if(TWI0.SSTATUS&TWI_APIF_bm){
		if(TWI0.SSTATUS&TWI_AP_bm){
			if(TWI0.SSTATUS&TWI_DIR_bm){
				// write
			}else{
				// recieve
				cnt=0;
			}
			TWI0.SCTRLB=TWI_SCMD_RESPONSE_gc;
		}else{
			// stop
			TWI0.SCTRLB=TWI_SCMD_COMPTRANS_gc;
		}
		TWI0.SSTATUS=TWI_APIF_bm;
	}

	if(TWI0.SSTATUS&TWI_DIF_bm){
		uint16_t r=TWI0.SDATA;
		uint16_t *d=cnt&0x10?disp_g:disp_r;

		d[(cnt&0xf)>>1]&=cnt&1?0xff00:0x00ff;
		d[(cnt&0xf)>>1]|=r<<(cnt&1?0:8);

		TWI0.SCTRLB=cnt++<32?TWI_SCMD_RESPONSE_gc:TWI_SCMD_COMPTRANS_gc;
		TWI0.SSTATUS=TWI_DIF_bm;
	}
}

void main(){
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,0);//CLKCTRL_PDIV_12X_gc|CLKCTRL_PEN_bm);

	// OE# CMP0
	TCA0.SINGLE.CTRLA=TCA_SINGLE_ENABLE_bm;
	TCA0.SINGLE.CTRLB=TCA_SINGLE_CMP0EN_bm|TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	TCA0.SINGLE.PER=255;
	TCA0.SINGLE.OE_CMP=0;

	TCB0.CTRLA=TCB_ENABLE_bm;
	TCB0.CCMP=U_SEC(20);

	TWI0.SADDR=ADDR<<1;
	TWI0.SCTRLA=TWI_DIEN_bm|TWI_APIEN_bm|TWI_ENABLE_bm;

	PORTA.DIRSET=_BV(SERCLK)|_BV(RCLK)|_BV(OE_);
	PORTA.OUTSET=_BV(SERCLK)|_BV(RCLK);

	sei();
	while(1)for(uint8_t i=0;i<16;++i){// 1300us
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
		PORTA.OUTCLR=_BV(RCLK);
		PORTA.OUTSET=_BV(RCLK);
		wait();
		// TCB0.CCMP=U_SEC(1000);
		// FOR(100)wait();
		// TCB0.CCMP=U_SEC(20);
	}
}
