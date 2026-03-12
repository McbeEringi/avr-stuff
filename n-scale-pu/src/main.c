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

#define F_SCL 400000UL
#define TWI_BAUD(X) (((F_CPU/(X))-10)/2)
#define LCD_ADDR 0x7c

void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}

static void led(uint8_t r,uint8_t g,uint8_t b){
	for(uint8_t i=0;i<3;++i){
		uint8_t x=((uint8_t[]){g,r,b})[i];
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
		if(t<0x8000>>2)PORTA.OUTSET=SPK;
		else PORTA.OUTCLR=SPK;
		t+=d;
		wait();
	}
	PORTA.OUTCLR=SPK;
}

const uint8_t neko[]={200, 186, 198, 197, 218, 217, 214, TERM};
const uint8_t nyan[]={198, 172, 176, 221, TERM};
static void TWI_begin(){TWI0.MADDR=LCD_ADDR;while(!(TWI0.MSTATUS&TWI_WIF_bm));}
static void TWI_write(uint8_t x){TWI0.MDATA=x;while(!(TWI0.MSTATUS&TWI_WIF_bm)||TWI0.MSTATUS&TWI_RXACK_bm);}
static void TWI_end(){TWI0.MCTRLB=TWI_MCMD_STOP_gc;}
static void LCD_cmd(const uint8_t x){TWI_begin();TWI_write(0);TWI_write(x);TWI_end();}
static void LCD_cmds(const uint8_t *p){
	TWI_begin();
	for(;*p;++p){
		TWI_write(*(p+1)?0x80:0);TWI_write(*p);
		if(*p>>3==0b1101)_delay_ms(200);
	}
	TWI_end();
}
static void LCD_init(){
	_delay_ms(50);
	LCD_cmds((const uint8_t[]){0x39,0x14,0x6c,0x38,0x0c,0x01,0x02,0});
	_delay_ms(2);
}
static void LCD_contrast(uint8_t c){
	LCD_cmds((const uint8_t[]){0x39,0x70|(c&0xf),0x54|((c>>4)&3),0x38,0});
}
static void print(const uint8_t *p){
	TWI_begin();
	TWI_write(0x40);for(;*p!=TERM;p++)TWI_write(*p);
	TWI_end();
}
static void cursor(uint8_t x,uint8_t y){LCD_cmd(0x80|(y==0?0:0x40)|(x&0x7));}
static void cgram(uint8_t i,const uint8_t *w){
	TWI_begin();
	TWI_write(0x80);TWI_write(0x40|((i&7)<<3));
	TWI_write(0x40);for(uint8_t j=0;j<8;++j)TWI_write(w[j]);
	TWI_end();
}

volatile uint8_t adc_state=0;
volatile uint8_t vsense=0;
volatile uint8_t vdd=0;
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
	if(adc_state){
		vsense=x*10/78;// v = x * 2.5/1023 * 24.7/4.7; x/v==78
	}else{
		// 0b010100; x == 706 @12V 3.6V
		// 0b011001; x == 816 @9V 3.1V
		LCD_contrast(0b010100+((x-699)*3>>6));
		vdd=25575/x;// v = 2.5/x*1023; v*x*10==25575
	}
	adc_state=!adc_state;
	adc_run();
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

int main() {
	_PROTECTED_WRITE(CLKCTRL_MCLKCTRLB,0);
	PORTMUX.CTRLC=PORTMUX_TCA00_ALTERNATE_gc;
	PORTA.DIRSET=NP|SPK;
	PORTB.DIRSET=MD_A|MD_B;

	TCB0.CTRLA=TCB_ENABLE_bm;
	TCB0.CCMP=F_CPU/4e4-1;// 40kHz

	TWI0.MBAUD=TWI_BAUD(F_SCL);
	TWI0.MCTRLA=TWI_ENABLE_bm;
	TWI0.MSTATUS=TWI_BUSSTATE_IDLE_gc;
	LCD_init();
	sei();
	adc_run();

	// cursor(0,0);print(neko);
	// cursor(0,1);print(nyan);print(nyan);
	cgram(0,zap);cgram(1,kmph_l);cgram(2,kmph_r);
	// cursor(0,1);print((const uint8_t[]){'6','0',' ',1,2,TERM});

	spk(3429,50);// 65536/20000*Hz
	spk(3849,50);
	spk(5138,50);
	spk(6858,50);

	uint8_t h=0;
	while(1){
		cli();
		for(uint8_t i=0;i<8;++i)led_hsv(h+(i<<5),128,16);
		sei();
		_delay_us(400);
		h+=4;
		cursor(0,0);print(n2str(vsense,(uint8_t[]){"\x00  x.x V\xa0"},2,2,3,5));
		cursor(0,1);print(n2str(vdd,(uint8_t[]){"VDD x.x V\xa0"},4,4,4,6));
		_delay_ms(50);
	}
}
