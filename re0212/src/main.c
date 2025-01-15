/*
IR: tX02 ? PA2 : PB2
LED: PA3
SW_A: PA7
SW_B: PA1
*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

// 01111011 10110101 01111101 11110111
// ==> 0xde     0xad     0xbe     0xef

// Hotalux RE0212
const uint8_t code_a[]={0x82,0x6d,0xbf,0x40};
const uint8_t code_b[]={0x82,0x6d,0x71,0x22};

// Panasonic HK9327K
// https://hello-world.blog.ss-blog.jp/2011-05-07
const uint8_t code_c[]={0x2c,0x52,0x09,0x2c,0x25};// 全灯
const uint8_t code_d[]={0x2c,0x52,0x09,0x2f,0x26};// 消灯


#define IR_HZ 38000
#define IR_TOP (F_CPU+IR_HZ/2)/IR_HZ
#define IR_ON TCA0.SINGLE.CMP2BUF=(IR_TOP+3/2)/3-1// duty=1/3
#define IR_OFF TCA0.SINGLE.CMP2BUF=0
#define LED_ON PORTA.OUTSET=1<<3
#define LED_OFF PORTA.OUTCLR=1<<3
#define FOR(X) for(uint8_t i=0;i<X;++i)
void set_wait(t){TCB0.CNT=0;TCB0.CCMP=(F_CPU*t+500000)/1000000-1;}// TCB0 TOP
void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}// TCB0

static void sleep(){sei();SLPCTRL.CTRLA=SLPCTRL_SMODE_PDOWN_gc|SLPCTRL_SEN_bm;sleep_cpu();cli();}
ISR(PORTA_PORT_vect){PORTA.INTFLAGS=PORT_INT7_bm|PORT_INT1_bm;}

static void send(const uint8_t *x,const uint8_t l,const uint8_t l_on,const uint8_t l_off){
	IR_ON;FOR(l_on)wait();IR_OFF;FOR(l_off)wait();
	FOR(l){IR_ON;wait();IR_OFF;wait();if(x[i>>3]>>(i&7)&1)FOR(2)wait();}
	IR_ON;wait();IR_OFF;
}
static void send_nec(x){set_wait(562);send(x,32,16,8);}
static void send_aeha(x,l){set_wait(425);send(x,l,8,4);}

void main(){
	// CLKCTRL CLR_PER=16M/12=1333k Hz
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,CLKCTRL_PDIV_12X_gc|CLKCTRL_PEN_bm);

	// TCA0 IR 1333kHz/35=38.095kHz
	TCA0.SINGLE.CTRLA=TCA_SINGLE_ENABLE_bm;
	TCA0.SINGLE.CTRLB=TCA_SINGLE_CMP2EN_bm|TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	TCA0.SINGLE.PER=IR_TOP-1;// TOP 35-1

	// TCB0 delay
	TCB0.CTRLA=TCB_ENABLE_bm;

	#ifdef USE_PB2
		PORTA.DIRSET=0b1000;// out: PA3
		PORTB.DIRSET=0b0100;// out: PB2
	#else
		PORTA.DIRSET=0b1100;// out: PA2,3
	#endif
	PORTA.PIN1CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL
	PORTA.PIN7CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL

	while(1){
		sleep();
		uint8_t f=0;
		if(~VPORTA.IN&(1<<7)){f=1;send_aeha(code_c,40);}
		if(~VPORTA.IN&(1<<1)){f=1;send_aeha(code_d,40);}
		if(f){LED_ON;wait();LED_OFF;}
	}
}
