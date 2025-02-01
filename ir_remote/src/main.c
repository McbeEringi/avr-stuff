// SW: PA6 PA7 PA1

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

// 01111011 10110101 01111101 11110111
// ==> 0xde     0xad     0xbe     0xef

// NEC Hotalux RE0212 (HLDC08301SG 付属)
const uint8_t nec_re_x[]={0x82,0x6d,
	// .+:ボタン名, .+!:反応あり, -:反応なし, --.+--:参考文献より引用 <https://shrkn65.nobody.jp/remocon/database.html>
	// TODO: リモコン設定用信号(中央3秒)
	//0xa0,0x5f// -
	//0xa1,0x5e// -
	//0xa2,0x5d// 点灯!
	//0xa3,0x5c// -
	//0xa4,0x5b// -
	//0xa5,0x5a// -
	//0xa6,0x59// 全灯
	//0xa7,0x58// 白色
	//0xa8,0x57// 暖色
	//0xa9,0x56// -
	//0xaa,0x55// -
	//0xab,0x54// --ON/OFF--
	//0xac,0x53// --アクティブ--
	//0xad,0x52// --ナチュラル--
	//0xae,0x51// --リラックス--
	//0xaf,0x50// スリープタイマー 60分/30分

	//0xb0,0x4f// --明るさ10-- (中略)
	//0xb9,0x46// --明るさ1--
	//0xba,0x45// 明るく
	//0xbb,0x44// 暗く
	//0xbc,0x43// 常夜灯!
	//0xbd,0x42// -
	//0xbe,0x41// OFF!
	//0xbf,0x40// 点灯 常夜灯 OFF
	
	//0x71,// 拡張領域?
		//0x19// 留守番
		//0x22// 残光(ホタルック)
};

const uint8_t nec_re_full[]={0x82,0x6d,0xa6,0x59};
const uint8_t nec_re_on[]={0x82,0x6d,0xa2,0x5d};
const uint8_t nec_re_dim[]={0x82,0x6d,0xbc,0x43};
const uint8_t nec_re_lumi[]={0x82,0x6d,0x71,0x22};
const uint8_t nec_re_off[]={0x82,0x6d,0xbe,0x41};

// AEHA Panasonic HK9327K
// https://hello-world.blog.ss-blog.jp/2011-05-07
const uint8_t pana_hk_up[]={0x2c,0x52,0x09,0x2a,0x23};// 明
const uint8_t pana_hk_dn[]={0x2c,0x52,0x09,0x2b,0x22};// 暗
const uint8_t pana_hk_full[]={0x2c,0x52,0x09,0x2c,0x25};// 全灯
const uint8_t pana_hk_on[]={0x2c,0x52,0x09,0x2d,0x24};// 点灯
const uint8_t pana_hk_dim[]={0x2c,0x52,0x09,0x2e,0x27};// 常夜灯
const uint8_t pana_hk_off[]={0x2c,0x52,0x09,0x2f,0x26};// 消灯

// SONY SONY RM-JD019
const uint8_t code_e[]={0x95,0};// TV PWR 12bit

// AEHA SHARP A020SD
const uint8_t code_f[]={0xaa,0x5a,0xcf,0x16,0x00,0x05,0x21,0xc1};// 点灯

// NEC IODATA HVT-T2RC2
const uint8_t code_g[]={0xca,0x5a,0x38,0xc7};// PWR
const uint8_t code_h[]={0xca,0x5a,0x35,0xca};// CHUP
const uint8_t code_i[]={0xca,0x5a,0x33,0xcc};// CHDN

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

#define LED_ON LED_PORT.OUTSET=1<<LED_PIN
#define LED_OFF LED_PORT.OUTCLR=1<<LED_PIN


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

static uint8_t ir_on;
#define IR_ON TCA0.SINGLE.IR_CMPnBUF=ir_on
#define IR_OFF TCA0.SINGLE.IR_CMPnBUF=0
static void set_38k_wait(t){
	// CLKCTRL CLR_PER=16M/12=1333k Hz
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,CLKCTRL_PDIV_12X_gc|CLKCTRL_PEN_bm);
	// TCA0 IR 1333kHz/35=38.095kHz
	ir_on=12;// CMP 35/3
	TCA0.SINGLE.PER=34;// TOP 35-1
	TCB0.CNT=0;TCB0.CCMP=(8*t+3)/6-1;// TOP round(us2top(t)) (1333333*t+500000)/1000000-1
}
static void set_40k_wait(t){
	// CLKCTRL CLR_PER=16M/16=1000k Hz
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,CLKCTRL_PDIV_16X_gc|CLKCTRL_PEN_bm);
	// TCA0 IR 1000kHz/25=40kHz
	ir_on=8;// CMP 25/3
	TCA0.SINGLE.PER=24;// TOP 25-1
	TCB0.CNT=0;TCB0.CCMP=(2*t+1)/2-1;// TOP round(us2top(t))-1 (1000000*t+500000)/1000000-1
}
static void set_36k_wait(t){
	// CLKCTRL CLR_PER=16M/12=1333k Hz
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,CLKCTRL_PDIV_12X_gc|CLKCTRL_PEN_bm);
	// TCA0 IR 1333kHz/37=36.036kHz
	ir_on=12;// CMP 37/3
	TCA0.SINGLE.PER=36;// TOP 37-1
	TCB0.CNT=0;TCB0.CCMP=(8*t+3)/6-1;// TOP round(us2top(t))-1 (1333333*t+500000)/1000000-1
}
static void set_56k_wait(t){
	// CLKCTRL CLR_PER=16M/48=333k Hz
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,CLKCTRL_PDIV_48X_gc|CLKCTRL_PEN_bm);
	// TCA0 IR 333kHz/6=55.5kHz
	ir_on=2;// CMP 6/3
	TCA0.SINGLE.PER=5;// TOP 6-1
	TCB0.CNT=0;TCB0.CCMP=(2*t+3)/6-1;// TOP round(us2top(t))-1 (333333*t+500000)/1000000-1
}



#define FOR(X) for(uint8_t i=0;i<X;++i)
#define FORBUF(X) for(uint8_t i=0,l=X;i<l;++i)
static void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}// TCB0

static void sleep(){sei();SLPCTRL.CTRLA=SLPCTRL_SMODE_PDOWN_gc|SLPCTRL_SEN_bm;sleep_cpu();cli();}
ISR(PORTA_PORT_vect){PORTA.INTFLAGS=PORT_INT6_bm|PORT_INT7_bm|PORT_INT1_bm;}

static void send_common(const uint8_t ll,const uint8_t *x,const uint8_t l){
	IR_ON;FOR(ll)wait();IR_OFF;FOR(ll/2)wait();
	FOR(l){IR_ON;wait();IR_OFF;wait();if(x[i>>3]>>(i&7)&1)FOR(2)wait();}
	IR_ON;wait();IR_OFF;
}
static void send_nec(const uint8_t *x){set_38k_wait(562);send_common(16,x,32);}
static void send_aeha(const uint8_t *x,const uint8_t l){set_38k_wait(425);send_common(8,x,l);}
static const uint8_t send_sony(
	const uint8_t *x,const uint8_t l
){
	uint8_t t=75-4-l*2;
	set_40k_wait(600);
	IR_ON;FOR(4)wait();IR_OFF;
	FOR(l){wait();IR_ON;wait();if(x[i>>3]>>(i&7)&1){--t;wait();}IR_OFF;}
	return t;
}

void main(){
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,CLKCTRL_PDIV_64X_gc|CLKCTRL_PEN_bm);
	TCA0.SINGLE.CTRLA=TCA_SINGLE_ENABLE_bm;
	TCA0.SINGLE.CTRLB=IR_CMP_BM|TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	TCB0.CTRLA=TCB_ENABLE_bm;
	TCB0.CCMP=250;// init any 250==1ms

	IR_PORT.DIRSET=1<<IR_PIN;
	LED_PORT.DIRSET=1<<LED_PIN;

	PORTA.PIN6CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL
	PORTA.PIN7CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL
	PORTA.PIN1CTRL=PORT_PULLUPEN_bm|PORT_ISC_BOTHEDGES_gc;// BOTHEDGES|LEVEL

	while(1){
		sleep();FOR(20)wait();
		const uint8_t x=~VPORTA.IN;
		uint8_t f=0;// ヒューズは書いたか?
		if(x&(1<<6))send_aeha(pana_hk_on,40);//{send_nec(code_g);FOR(150)wait();FOR(4)FORBUF(send_sony(code_e,12))wait();}//send_aeha(code_f,64);
		else if(x&(1<<7))send_aeha(pana_hk_dim,40);
		else if(x&(1<<1))send_aeha(pana_hk_off,40);
		else ++f;
		if(!f){LED_ON;wait();LED_OFF;}
	}
}
