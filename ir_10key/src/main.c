// SW: PA6 PA7 PA1

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

// 01111011 10110101 01111101 11110111
// ==> 0xde     0xad     0xbe     0xef


#define IR_WO 0

#define PA1_CODE 9
#define PA2_CODE 6
#define PA3_CODE 0
#define PA4_CODE 3
#define PA5_CODE 7
#define PA6_CODE 5
#define PA7_CODE 2
#define PB1_CODE 8
#define PB2_CODE 1
#define PB3_CODE 4

////////////////////////////////////////////////////////////



#if defined(PORTB)&&!defined(PORTF)
	#define TCA0_IS_PORTB
	#define IR_PORT PORTB
#else
	#define IR_PORT PORTA
#endif

#if IR_WO==0
	#ifdef TCA0_IS_PORTB
		#define IR_PIN 0
	#else
		#define IR_PIN 3
	#endif
	#define IR_CMP_BM TCA_SINGLE_CMP0EN_bm
	#define IR_CMPnBUF CMP0BUF
#elif IR_WO==1
	#define IR_PIN IR_WO
	#define IR_CMP_BM TCA_SINGLE_CMP1EN_bm
	#define IR_CMPnBUF CMP1BUF
#elif IR_WO==2
	#define IR_PIN IR_WO
	#define IR_CMP_BM TCA_SINGLE_CMP2EN_bm
	#define IR_CMPnBUF CMP2BUF
#else
	#error "IR_WO out of 0 ~ 2"
#endif

#define IR_ON TCA0.SINGLE.IR_CMPnBUF=12 // CMP 35/3
#define IR_OFF TCA0.SINGLE.IR_CMPnBUF=0


#define FOR(X) for(uint8_t i=0;i<X;++i)
static void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}// TCB0

static void sleep(){sei();SLPCTRL.CTRLA=SLPCTRL_SMODE_PDOWN_gc|SLPCTRL_SEN_bm;sleep_cpu();cli();}
ISR(PORTA_PORT_vect){PORTA.INTFLAGS=0b11111110;}
ISR(PORTB_PORT_vect){PORTB.INTFLAGS=0b1110;}

static void send(const uint8_t *x){
	IR_ON;FOR(16)wait();IR_OFF;FOR(8)wait();
	FOR(32){IR_ON;wait();IR_OFF;wait();if(x[i>>3]>>(i&7)&1)FOR(2)wait();}
	IR_ON;wait();IR_OFF;
}

void main(){
	// CLKCTRL CLR_PER=16M/12=1333k Hz
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,CLKCTRL_PDIV_12X_gc|CLKCTRL_PEN_bm);
	TCA0.SINGLE.CTRLA=TCA_SINGLE_ENABLE_bm;
	TCA0.SINGLE.CTRLB=IR_CMP_BM|TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	// TCA0 IR 1333kHz/35=38.095kHz
	TCA0.SINGLE.PER=34;// TOP 35-1
	TCB0.CTRLA=TCB_ENABLE_bm;
	TCB0.CNT=0;TCB0.CCMP=748;// TOP round(us2top(t)) (1333333*t+500000)/1000000-1 t=562 (8*562)/6-1

	IR_PORT.DIRSET=1<<IR_PIN;

	PORTA.PIN1CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL
	PORTA.PIN2CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL
	PORTA.PIN3CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL
	PORTA.PIN4CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL
	PORTA.PIN5CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL
	PORTA.PIN6CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL
	PORTA.PIN7CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL
	PORTB.PIN1CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL
	PORTB.PIN2CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL
	PORTB.PIN3CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL

	uint16_t prev=0;
	while(1){
		// ヒューズは書いたか?
		sleep();FOR(20)wait();
		// B  A
		// 3217654321
		const uint16_t now=(((~VPORTB.IN>>1)&0b111)<<7)|((~VPORTA.IN>>1)&0x7f);
		const uint16_t y=(prev^now)&now;
		prev=now;

		if(y){
			uint8_t x=
				(y&(1<<0))?PA1_CODE:
				(y&(1<<1))?PA2_CODE:
				(y&(1<<2))?PA3_CODE:
				(y&(1<<3))?PA4_CODE:
				(y&(1<<4))?PA5_CODE:
				(y&(1<<5))?PA6_CODE:
				(y&(1<<6))?PA7_CODE:
				(y&(1<<7))?PB1_CODE:
				(y&(1<<8))?PB2_CODE:
				(y&(1<<9))?PB3_CODE:
				255;

			send((uint8_t[]){0x6c,0xa8,x,~x});
			// IR_ON;FOR(127)wait();
		}
		IR_OFF;
	}
}
