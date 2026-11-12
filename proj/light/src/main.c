#include <avr/io.h>
#include <avr/sleep.h>

#define BTN_PIN 6

#define BTN_DOWN (~VPORTA.IN&_BV(BTN_PIN))
#define FOR(N) for(uint8_t i=0;i<N;i++)

const uint16_t lower=0x1,upper=0x200;
uint16_t lum=0x40;
uint8_t dir=1;

uint8_t max(uint8_t a,uint8_t b){return a<b?b:a;}
uint8_t lerp(uint16_t dst){
	uint8_t f=0;
	if(dst!=lum){if(lum<dst){
		lum+=max(lum>>4,1);if(dst<=lum)f=1;
	}else{
		lum-=max(lum>>4,1);if(lum<=dst)f=1;
	}}
	if(f)lum=dst;
	TCA0.SINGLE.CMP0BUF=lum;
	return f;
}

void main(){
	// OFF
	PORTA.DIRSET=_BV(7);SLPCTRL.CTRLA=0b101;sleep_cpu();

	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,CLKCTRL_PDIV_4X_gc|CLKCTRL_PEN_bm);// 4MHz

	PORTMUX.CTRLC=PORTMUX_TCA00_bm;
	TCA0.SINGLE.CTRLA=TCA_SINGLE_ENABLE_bm;
	TCA0.SINGLE.CTRLB=TCA_SINGLE_CMP0EN_bm|TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	TCA0.SINGLE.PER=0x3ff;// TOP 10bit
	TCA0.SINGLE.CMP0BUF=lum;

	TCB0.CTRLA=TCB_ENABLE_bm;
	TCB0.CCMP=39999;// TOP 40000/4MHz=10ms

	PORTA.DIRSET=_BV(7);
	PORTA.PIN6CTRL=PORT_PULLUPEN_bm;

	while(1){
		if(BTN_DOWN&&lerp(dir?upper:lower))dir=!dir;
		FOR(4){while(!TCB0.INTFLAGS);TCB0.INTFLAGS=1;}
	}
}
