#include <avr/io.h>
#include <util/delay.h>

#define NP (1<<1)
#define A {asm("nop");asm("nop");asm("nop");}
#define B {A;A;A;A;}

void led(uint8_t r,uint8_t g,uint8_t b){
	for(uint8_t i=0;i<3;++i){
		uint8_t x=((uint8_t[]){g,r,b})[i];
		for(uint8_t m=0x80;m;m>>=1){
			if(x&m){
				PORTA.OUTSET=NP;B;
			}else{
				PORTA.OUTSET=NP;A;
			}
			PORTA.OUTCLR=NP;B;
		}
	}
}

int main() {
	_PROTECTED_WRITE(CLKCTRL_MCLKCTRLB,0);
	PORTA.DIRSET=NP;
	uint16_t c=0;
	while(1){
		for(uint8_t i=0;i<9;++i){
			if(c&(0x100>>i))led(0,16,0);
			else
				led(8,0,0);
		}
		_delay_us(400);
		c=++c&0x01ff;
		_delay_ms(5);
	}
}
