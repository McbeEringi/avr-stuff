/*
IR: tX02 ? PA2 : PB2
LED: PA3
SW_A: PA7
SW_B: PA1
*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>


#define CODE_A 0x41b6fd02
const uint8_t code_a[]={0x82,0x6d,0xbf,0x40};
#define CODE_B 0x41b68e44
const uint8_t code_b[]={0x82,0x6d,0x71,0x22};

// https://hello-world.blog.ss-blog.jp/2011-05-07
const uint8_t code_c[]={0x2c,0x52,0x09,0x2c,0x25};
const uint8_t code_d[]={0x2c,0x52,0x09,0x2f,0x26};

#define IR_HZ 38000
#define T 435//562
#define LEADER_ON 8//16
#define LEADER_OFF 4//8



#define IR_TOP (F_CPU+IR_HZ/2)/IR_HZ
#define IR_ON TCA0.SINGLE.CMP0BUF=(IR_TOP+3/2)/3-1// duty=1/3
#define IR_OFF TCA0.SINGLE.CMP0BUF=0
#define LED_ON PORTA.OUTSET=1<<3
#define LED_OFF PORTA.OUTCLR=1<<3
#define FOR(X) for(uint8_t i=0;i<X;++i)
#define FOR_(X) for(uint8_t j=0;j<X;++j)
void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}// TCB0 T us

static void sleep(){sei();SLPCTRL.CTRLA=SLPCTRL_SMODE_PDOWN_gc|SLPCTRL_SEN_bm;sleep_cpu();cli();}
ISR(PORTA_PORT_vect){PORTA.INTFLAGS=PORT_INT7_bm|PORT_INT1_bm;}

/*static void send(uint32_t x){*/
/*	IR_ON;FOR(LEADER_ON)wait();IR_OFF;FOR(LEADER_OFF)wait();*/
/*	FOR(32){IR_ON;wait();IR_OFF;wait();if(x>>(31-i)&1)FOR(2)wait();}*/
/*	IR_ON;wait();IR_OFF;*/
/*}*/
static void send(const uint8_t *x,uint8_t l){
	IR_ON;FOR(LEADER_ON)wait();IR_OFF;FOR(LEADER_OFF)wait();
	FOR(l){FOR_(8){IR_ON;wait();IR_OFF;wait();if(x[i]>>j&1)FOR(2)wait();}}
	IR_ON;wait();IR_OFF;
}

void main(){
	// TCA0 IR 1333kHz/35=38.095kHz
	TCA0.SINGLE.CTRLA=TCA_SINGLE_ENABLE_bm;
	TCA0.SINGLE.CTRLB=TCA_SINGLE_CMP0EN_bm|TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	TCA0.SINGLE.PER=IR_TOP-1;// TOP 35-1

	// TCB0 delay 1333kHz/749=1780.151Hz T us
	TCB0.CTRLA=TCB_ENABLE_bm;
	TCB0.CCMP=(F_CPU*T+500000)/1000000-1;// TOP 749-1

	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,CLKCTRL_PDIV_12X_gc|CLKCTRL_PEN_bm);// CLR_PER=16M/12=1333k Hz
	#ifdef USE_PB2
		PORTA.DIRSET=0b1000;// out: PA3
		PORTB.DIRSET=0b0100;// out: PB2
	#else
		PORTA.DIRSET=0b1001;// out: PA2,3
	#endif
	PORTA.PIN1CTRL=PORT_PULLUPEN_bm|PORT_ISC_LEVEL_gc;// BOTHEDGES|LEVEL
	PORTA.PIN7CTRL=PORT_PULLUPEN_bm|PORT_ISC_LEVEL_gc;// BOTHEDGES|LEVEL

	LED_ON;FOR(255)wait();LED_OFF;

	while(1){
		sleep();
		if(~VPORTA.IN&(1<<7))send(code_c,5);//send(code_a,4);//send(CODE_A);
		if(~VPORTA.IN&(1<<1))send(code_d,5);//send(code_b,4);//send(CODE_B);
		LED_ON;wait();LED_OFF;
		while(~VPORTA.IN&(1<<7|1<<1))FOR(18)wait();// about 10ms
	}
}
