#include <avr/io.h>

#define BTN_DOWN ~VPORTA.IN&(1<<7)
uint16_t lower=0x10,upper=0x100,lum=0x40;
uint8_t dir=1;

void main(){
	TCA0.SINGLE.CTRLA=TCA_SINGLE_ENABLE_bm;// 分周無し TCA有効
	TCA0.SINGLE.CTRLB=TCA_SINGLE_CMP2EN_bm|TCA_SINGLE_WGMODE_SINGLESLOPE_gc;// TCA0 wo2有効 単傾斜PWM
	TCA0.SINGLE.PER=0x3ff;// TOP 10bit
	TCA0.SINGLE.CMP2BUF=lum;

	TCB0.CTRLA=TCB_CLKSEL_CLKDIV2_gc|TCB_ENABLE_bm;// 1/2 TCB有効
	TCB0.CCMP=49999;// TOP 20MHz/2*50000=5ms

	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,0);// 20MHz
	PORTA.DIRSET=0b1100;// 出力: PA3,2
	PORTA.PIN7CTRL=PORT_PULLUPEN_bm;// PA7 プルアップ

	while(1){
		if(BTN_DOWN){
			if(dir)lum+=lum>>4;else lum-=lum>>4;
			if(lum<lower){lum=lower;dir=1;}else if(upper<lum){lum=upper;dir=0;}
			TCA0.SINGLE.CMP2BUF=lum;
		}
		for(uint8_t i=0;i<4;i++){while(!TCB0.INTFLAGS);TCB0.INTFLAGS=1;}// TCB0待ち 1書いて解除
	}
}