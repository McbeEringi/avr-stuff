#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#define F_SCL 400000UL
#define TWI_BAUD(X) (((F_CPU/(X))-10)/2)
#define UART_BAUD(X) (4.*F_CPU/(X)+.5)
#define FOR(X) for(uint8_t i=0;i<X;i++)
#define U_SEC(X) (F_CPU/1000000*(X)-1)
void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}



const uint8_t w[]={
	142,10,238,188,32,28,158,65,65,65,0,122,58,0,42,58,30,0,30,58,56,26,46,65,65,65,0,0,0,0
	// 110,158,28,28,252,0,124,158,156,65,0,0,0,0
};

const uint8_t addr[]={
	0x10,0x11,0x12,0x13,
	0x14,0x15,0x16,0x17
};

void TWI_begin(uint8_t addr){
	TWI0.MADDR=(addr<<1)|0; // Write
	while(!(TWI0.MSTATUS&TWI_WIF_bm));
}
void TWI_write(uint8_t x){
	TWI0.MDATA=x;
	while(!(TWI0.MSTATUS&TWI_WIF_bm));
}
void TWI_end(){TWI0.MCTRLB=TWI_MCMD_STOP_gc;}


int main(void) {
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,0);

	TCB0.CTRLA=TCB_ENABLE_bm;
	TCB0.CCMP=U_SEC(2000);

	TWI0.MBAUD=TWI_BAUD(F_SCL);
	TWI0.MCTRLA=TWI_ENABLE_bm;
	TWI0.MSTATUS=TWI_BUSSTATE_IDLE_gc;

	uint8_t c=0;
	while(1){
		for(uint8_t i=0;i<sizeof(addr);++i){
			TWI_begin(addr[i]);
			for(uint8_t j=0;j<8;++j)TWI_write(w[(c+i*8+j)%sizeof(w)]);
			TWI_write(0xaa);TWI_write(0xaa);
			TWI_end();
		}

		FOR(100)wait();// 0.2s

		++c;
		if(sizeof(w)<=c)c=0;
	}
}
