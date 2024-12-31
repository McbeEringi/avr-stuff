/*
IR: tX02 ? PA2 : PB2
LED: PA3
SW : PA7
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define CODE 0x41b6fd02

#define BTN_DOWN ~VPORTA.IN&(1<<7)
#define FOR(X) for(uint8_t i=0;i<X;i++)
#define IR_ON TCA0.SINGLE.CMP2BUF=10// 35/3-1
#define IR_OFF TCA0.SINGLE.CMP2BUF=0
void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}// TCB0 561.75us
void wait_btn(){while(BTN_DOWN)FOR(18)wait();}// 0.56175*18=10.1115ms

void sleep(){sei();SLPCTRL.CTRLA=SLPCTRL_SMODE_PDOWN_gc|SLPCTRL_SEN_bm;sleep_cpu();cli();}
ISR(PORTA_PORT_vect){PORTA.INTFLAGS=PORT_INT7_bm;}

static void blink(uint8_t x){FOR(8){if((x>>i)&1)PORTA.OUTSET=0b1000;else PORTA.OUTCLR=0b1000;FOR(111)wait();}}// LSB first 1/16s * 8

static void send(uint32_t x){
	IR_ON;FOR(16)wait();
	IR_OFF;FOR(8)wait();
	FOR(32){
		IR_ON;wait();IR_OFF;wait();
		if(x>>(31-i)&1){wait();wait();}
	}
	IR_ON;wait();IR_OFF;
}

void main(){
	// TCA0 IR 1333kHz/35=38.095kHz
	TCA0.SINGLE.CTRLA=TCA_SINGLE_ENABLE_bm;// 分周無し TCA有効
	TCA0.SINGLE.CTRLB=TCA_SINGLE_CMP2EN_bm|TCA_SINGLE_WGMODE_SINGLESLOPE_gc;// TCA0 wo2有効 単傾斜PWM
	TCA0.SINGLE.PER=34;// TOP 35-1

	// TCB0 delay 1333kHz/749=1780.151Hz=561.75us
	TCB0.CTRLA=TCB_ENABLE_bm;// 分周無し TCB有効
	TCB0.CCMP=748;// TOP=F_CPU/1780=(749.333-1)

	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,CLKCTRL_PDIV_12X_gc|CLKCTRL_PEN_bm);// 12分周 CLR_PER=16M/12=1333k Hz
	#ifdef USE_PB2
		PORTA.DIRSET=0b1000;// 出力: PA3
		PORTB.DIRSET=0b0100;// 出力: PB2
	#else
		PORTA.DIRSET=0b1100;// 出力: PA2,3
	#endif
	PORTA.PIN7CTRL=PORT_PULLUPEN_bm|PORT_ISC_LEVEL_gc;// PA7 プルアップ BOTHEDGESかLEVEL

	blink(0b01010101);
	while(1){sleep();send(CODE);blink(0b00110011);wait_btn();}
}
