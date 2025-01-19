// SW: PA6 PA7 PA1

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
#define IR_WO 0
#define LED_PORT PORTA
#define LED_PIN 2


#ifdef REV1
	#define IR_WO 2
	#define LED_PORT PORTA
	#define LED_PIN 3
#endif
#ifdef REV2
	#define IR_WO 0
	#define LED_PORT PORTA
	#define LED_PIN 2
#endif

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


#define IR_TOP (F_CPU+IR_HZ/2)/IR_HZ
#define IR_ON TCA0.SINGLE.IR_CMPnBUF=(IR_TOP+3/2)/3-1// duty=1/3
#define IR_OFF TCA0.SINGLE.IR_CMPnBUF=0
#define LED_ON LED_PORT.OUTSET=1<<LED_PIN
#define LED_OFF LED_PORT.OUTCLR=1<<LED_PIN
#define FOR(X) for(uint8_t i=0;i<X;++i)
void set_wait(t){TCB0.CNT=0;TCB0.CCMP=(F_CPU*t+500000)/1000000-1;}// TCB0 TOP
void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}// TCB0

static void sleep(){sei();SLPCTRL.CTRLA=SLPCTRL_SMODE_PDOWN_gc|SLPCTRL_SEN_bm;sleep_cpu();cli();}
ISR(PORTA_PORT_vect){PORTA.INTFLAGS=PORT_INT6_bm|PORT_INT7_bm|PORT_INT1_bm;}

static void send(const uint8_t *x,const uint8_t l,const uint8_t l_on,const uint8_t l_off){
	IR_ON;FOR(l_on)wait();IR_OFF;FOR(l_off)wait();
	FOR(l){IR_ON;wait();IR_OFF;wait();if(x[i>>3]>>(i&7)&1)FOR(2)wait();}
	IR_ON;wait();IR_OFF;LED_ON;
}
static void send_nec(x){set_wait(562);send(x,32,16,8);}
static void send_aeha(x,l){set_wait(425);send(x,l,8,4);}

void main(){
	// CLKCTRL CLR_PER=16M/12=1333k Hz
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,CLKCTRL_PDIV_12X_gc|CLKCTRL_PEN_bm);

	// TCA0 IR 1333kHz/35=38.095kHz
	TCA0.SINGLE.CTRLA=TCA_SINGLE_ENABLE_bm;
	TCA0.SINGLE.CTRLB=IR_CMP_BM|TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	TCA0.SINGLE.PER=IR_TOP-1;// TOP 35-1

	// TCB0 delay
	TCB0.CTRLA=TCB_ENABLE_bm;
	TCB0.CCMP=1000;// init any

	IR_PORT.DIRSET=1<<IR_PIN;
	LED_PORT.DIRSET=1<<LED_PIN;

	PORTA.PIN6CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL
	PORTA.PIN7CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL
	PORTA.PIN1CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL

	while(1){
		sleep();FOR(20)wait();
		const uint8_t x=~VPORTA.IN;
		if(x&(1<<6))send_nec(code_a);
		else if(x&(1<<7))send_nec(code_a);
		else if(x&(1<<1))send_nec(code_b);//send_aeha(code_d,40);
		wait();LED_OFF;
	}
}
