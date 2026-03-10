#include <avr/io.h>
#include <util/delay.h>

#define NP (1<<2)
#define SPK (1<<5)
#define BTN_LU (1<<4)
#define BTN_LD (1<<3)
#define BTN_RU (1<<6)
#define BTN_RD (1<<1)
#define V_SENSE (1<<7)
#define A {asm("nop");asm("nop");asm("nop");}
#define B {A;A;A;A;}

#define F_SCL 400000UL
#define TWI_BAUD(X) (((F_CPU/(X))-10)/2)
#define WAIT_TWI_WIF() do{}while(!(TWI0.MSTATUS&TWI_WIF_bm)&&(TWI0.MSTATUS&TWI_RXACK_bm))
#define LCD_ADDR 0x7c
#define LCD_CONTRAST 0b010100


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
static void tone(uint8_t d,uint16_t ms){
	for(uint16_t t=0;t<ms;++t){
		PORTA.OUTSET=SPK;_delay_us(1);
		PORTA.OUTCLR=SPK;_delay_us(d);
	}
}

static void TWI_begin(){TWI0.MADDR=LCD_ADDR;WAIT_TWI_WIF();}
static void TWI_write(uint8_t x){TWI0.MDATA=x;WAIT_TWI_WIF();}
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
static void print(const char *p){
	TWI_begin();
	TWI_write(0x40);for(;*p;p++)TWI_write(*p);
	TWI_end();
}
static void cursor(uint8_t x,uint8_t y){LCD_cmd(0x80|(y==0?0:0x40)|(x&0x7));}

const char neko[]={200, 186, 198, 197, 218, 217, 214, 32, 0};
const char nyan[]={198, 172, 176, 221, 0};

int main() {
	_PROTECTED_WRITE(CLKCTRL_MCLKCTRLB,0);
	PORTMUX.CTRLC=PORTMUX_TCA00_ALTERNATE_gc;
	PORTA.DIRSET=NP|SPK;

	TWI0.MBAUD=TWI_BAUD(F_SCL);
	TWI0.MCTRLA=TWI_ENABLE_bm;
	TWI0.MSTATUS=TWI_BUSSTATE_IDLE_gc;
	LCD_init();
	LCD_contrast(LCD_CONTRAST);

	cursor(0,0);print(neko);
	cursor(0,1);print(nyan);print(nyan);


	uint8_t h=0;
	while(1){
		for(uint8_t i=0;i<8;++i)led_hsv(h+(i<<5),255,16);
		_delay_us(400);
		++h;
		_delay_ms(50);
	}
}
