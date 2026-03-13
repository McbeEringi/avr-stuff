#include <stddef.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "glyph.h"

#define NP (1<<2)
#define SPK (1<<5)
#define BTN_LU (1<<4)
#define BTN_LD (1<<3)
#define BTN_RU (1<<6)
#define BTN_RD (1<<1)
#define V_SENSE (1<<7)
#define MD_A (1<<2)
#define MD_B (1<<3)
#define A {asm("nop");asm("nop");}
#define B {A;A;A;A;A;A;A;A;}
#define TERM 0xa0

#define F_SCL 200000UL
#define TWI_BAUD(X) (((F_CPU/(X))-10)/2)
#define LCD_ADDR 0x7c

void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}

static void led(uint8_t r,uint8_t g,uint8_t b){
	for(uint8_t i=0;i<3;++i){
		uint8_t x=i==0?g:i==1?r:b;
		for(uint8_t m=0x80;m;m>>=1){
			if(x&m){PORTA.OUTSET=NP;B;}
			else{PORTA.OUTSET=NP;A;}
			PORTA.OUTCLR=NP;B;
		}
	}
}
static void led_hsv(uint8_t h,uint8_t s,uint8_t v){
	uint8_t f=((255-s)*v)>>8,w=v-f,o=h%85,io=84-o,u=(o*w)/85+f,d=(io*w)/85+f;
	switch(h/85){
		case 1:{led(f,d,u);break;}
		case 2:{led(u,f,d);break;}
		default:{led(d,u,f);break;}
	}
}
static void spk(uint16_t d,uint16_t n){
	n*=40;
	for(uint16_t i=0,t=0;i<n;++i){
		if(t<0x8000)PORTA.OUTSET=SPK;
		else PORTA.OUTCLR=SPK;
		t+=d;
		wait();
	}
	PORTA.OUTCLR=SPK;
}

const uint8_t neko[]={200, 186, 198, 197, 218, 217, 214, TERM};
const uint8_t nyan[]={198, 172, 176, 221, TERM};
static void _TWI_wait(){
	while(!(TWI0.MSTATUS&(TWI_WIF_bm|TWI_BUSERR_bm|TWI_ARBLOST_bm)));
	if(TWI0.MSTATUS&(TWI_BUSERR_bm|TWI_ARBLOST_bm))
		TWI0.MSTATUS=TWI_BUSERR_bm|TWI_ARBLOST_bm;
}
static void TWI_begin(){TWI0.MSTATUS=TWI_BUSSTATE_IDLE_gc;TWI0.MADDR=LCD_ADDR;_TWI_wait();}
static void TWI_write(uint8_t x){TWI0.MDATA=x;_TWI_wait();}
static void TWI_end(){TWI0.MCTRLB=TWI_MCMD_STOP_gc;}

static void send(const uint8_t *cmds,const uint8_t *data){
	TWI_begin();
	if(cmds){for(;*cmds;++cmds){
		TWI_write(data||*(cmds+1)?0x80:0);TWI_write(*cmds);
		if(*cmds>>3==0b1101)_delay_ms(200);else if(*cmds==1)_delay_ms(2);
	}}
	if(data){TWI_write(0x40);for(;*data!=TERM;data++)TWI_write(*data);}
	TWI_end();
}


static void send_init(){
	_delay_ms(50);
	send((const uint8_t[]){0x39,0x14,0x6c,0x38,0x0c,0x01,0x02,0},NULL);
}
static void send_contrast(uint8_t c){send((const uint8_t[]){0x39,0x70|(c&0xf),0x54|((c>>4)&3),0x38,0},NULL);}
static uint8_t cursor(uint8_t x,uint8_t y){return 0x80|(y<<6)|(x&7);}
static void cgram(uint8_t i,const uint8_t *w){
	TWI_begin();
	TWI_write(0x80);TWI_write(0x40|((i&7)<<3));
	TWI_write(0x40);for(uint8_t j=0;j<8;++j)TWI_write(w[j]);
	TWI_end();
}
static void shutdown(){
	// for(uint8_t i=0;i<8;++i)led(0,0,0);
	send((uint8_t[]){cursor(0,0),0},"see you!\xa0");
	send((uint8_t[]){cursor(0,1),0},"        \xa0");
	spk(1714,100);
	cli();
	exit(0);
}


volatile uint8_t adc_state=0;
volatile uint8_t vsense=0;
volatile uint8_t vdd=0;
volatile uint8_t tcc=0;
static void adc_run(){
	VREF.CTRLA=VREF_ADC0REFSEL_2V5_gc;
	ADC0.INTCTRL = ADC_RESRDY_bm;
	if(adc_state){// vsense
		ADC0.MUXPOS=ADC_MUXPOS_AIN7_gc;
		ADC0.CTRLC=ADC_REFSEL_INTREF_gc;
	}else{// vdd
		ADC0.MUXPOS=ADC_MUXPOS_INTREF_gc;
		ADC0.CTRLC=ADC_REFSEL_VDDREF_gc;
	}
	ADC0.CTRLC|=ADC_SAMPCAP_bm|ADC_PRESC_DIV256_gc;// 50k < CLK_ADC < 1.5M
	ADC0.CTRLB=ADC_SAMPNUM_ACC64_gc;
	ADC0.CTRLA=ADC_ENABLE_bm;
	ADC0.COMMAND=ADC_STCONV_bm;
}
ISR(ADC0_RESRDY_vect){
	uint16_t x=ADC0.RES>>6;
	if(adc_state)vsense=x*10/78;// v = x * 2.5/1023 * 24.7/4.7; x/v==78
	else{
		// 0b010100; x == 706 @12V 3.6V
		// 0b011001; x == 816 @9V 3.1V
		if(!tcc++){send_contrast(0b010100+((x-699)*3>>6));spk(3429,10);}
		vdd=25575/x;// v = 2.5/x*1023; v*x*10==25575
	}
	if(vdd<30)shutdown();
	else{
		adc_state=!adc_state;
		adc_run();
	}
}

static uint8_t *n2str(uint16_t x,uint8_t *w,const uint8_t p3,const uint8_t p2,const uint8_t p1,const uint8_t p0){
	uint8_t t,b=0;
	for(t=0;x>=1000;x-=1000,t++);
	if(t||b)w[p3]='0'+t;if(t)b=1;
	for(t=0;x>= 100;x-= 100,t++);
	if(t||b)w[p2]='0'+t;if(t)b=1;
	for(t=0;x>=  10;x-=  10,t++);
	if(t||b)w[p1]='0'+t;if(t)b=1;
	w[p0]='0'+x;
	return w;
}

uint8_t btn;
static uint8_t btn_down(uint8_t x){return(btn^PORTA.IN)&PORTA.IN&x;}

int main() {
	_PROTECTED_WRITE(CLKCTRL_MCLKCTRLB,0);
	PORTMUX.CTRLC=PORTMUX_TCA00_ALTERNATE_gc;
	PORTA.DIRSET=NP|SPK;
	PORTB.DIRSET=MD_A|MD_B;
	btn=PORTA.IN;

	PORTA.PIN1CTRL=
	PORTA.PIN3CTRL=
	PORTA.PIN4CTRL=
	PORTA.PIN6CTRL=PORT_PULLUPEN_bm;

	TCB0.CTRLA=TCB_ENABLE_bm;
	TCB0.CCMP=F_CPU/4e4-1;// 40kHz

	TWI0.MBAUD=TWI_BAUD(F_SCL);
	TWI0.MCTRLA=TWI_ENABLE_bm;
	send_init();
	sei();
	adc_run();

	send((uint8_t[]){cursor(0,0),0},"hello!\xa0");

	spk(3429,50);// 65536/20000*Hz
	spk(3849,50);
	spk(5138,50);
	spk(6858,50);
	_delay_ms(500);

	uint8_t t=0;
	uint8_t state=0;
	uint8_t pow=0;
	while(1){
		switch(state){
			case 0x00:{
				send((uint8_t[]){1,0},NULL);
				cgram(0,zap);_delay_ms(1);cgram(1,mcu);
				++state;
			}
			case 0x01:{
				cli();
				for(uint8_t i=0;i<8;++i)led_hsv(t+(i<<5),128,16);
				sei();
				_delay_us(400);
				send((uint8_t[]){cursor(0,0),0},n2str(vsense,(uint8_t[]){"\x00  x.x V\xa0"},2,2,3,5));
				send((uint8_t[]){cursor(0,1),0},n2str(   vdd,(uint8_t[]){"\x01  x.x V\xa0"},3,3,3,5));
				if(btn_down(BTN_RU))state|=0xf;
				break;
			}
			case 0x0f:{spk(3429,20);state=0x10;break;}

			case 0x10:{
				send((uint8_t[]){1,0},NULL);
				cli();
				for(uint8_t i=0;i<8;++i)led(8,4,4);
				sei();
				++state;
			}
			case 0x11:{
				send((uint8_t[]){cursor(0,0),0},"PWM     \xa0");
				send((uint8_t[]){cursor(0,1),0},n2str(pow,(uint8_t[]){"  x/255 \xa0"},0,0,1,2));
				if(btn_down(BTN_LU))++pow;
				if(btn_down(BTN_LD))--pow;
				if(btn_down(BTN_RU))state|=0xf;
				break;
			}
			case 0x1f:{spk(4322,20);state=0x20;break;}

			case 0x20:{
				send((uint8_t[]){1,0},NULL);
				cli();
				for(uint8_t i=0;i<8;++i)led(4,8,4);
				sei();
				++state;
			}
			case 0x21:{
				send((uint8_t[]){cursor(0,0),0},"0x21\xa0");
				if(btn_down(BTN_RU))state|=0xf;
				break;
			}
			case 0x2f:{spk(5138,20);state=0x30;break;}

			case 0x30:{
				send((uint8_t[]){1,0},NULL);
				cli();
				for(uint8_t i=0;i<8;++i)led(4,4,8);
				sei();
				++state;
			}
			case 0x31:{
				send((uint8_t[]){cursor(0,0),0},"0x31\xa0");
				if(btn_down(BTN_RU))state|=0xf;
				break;
			}
			case 0x3f:{spk(5767,20);state=0x40;break;}

			case 0x40:{
				send((uint8_t[]){1,0},NULL);
				cli();
				for(uint8_t i=0;i<8;++i)led(0,8,8);
				sei();
				++state;
			}
			case 0x41:{
				send((uint8_t[]){cursor(0,0),0},"0x41\xa0");
				if(btn_down(BTN_RU))state|=0xf;
				break;
			}
			case 0x4f:{spk(6473,20);state=0x50;break;}

			case 0x50:{
				send((uint8_t[]){1,0},NULL);
				cli();
				for(uint8_t i=0;i<8;++i)led(8,0,8);
				sei();
				++state;
			}
			case 0x51:{
				send((uint8_t[]){cursor(0,0),0},"0x51\xa0");
				if(btn_down(BTN_RU))state|=0xf;
				break;
			}
			case 0x5f:{spk(3237,20);_delay_ms(50);spk(3237,20);state=0x00;break;}

			default:shutdown();
		}
		btn=PORTA.IN;
		++t;
		_delay_ms(50);
	}
}
