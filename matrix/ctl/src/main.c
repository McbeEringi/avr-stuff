// #include <Wire.h>        //(1)
//
// void setup() {
//   Wire.begin();         //(2)
// }
//
//
// void loop() {
//   Wire.beginTransmission(0x12);         //(6)
//   Wire.write(1);        //(7)
//   Wire.endTransmission();        //(8)
//
//   delay(500);
// }
#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#define FOR(X) for(uint8_t i=0;i<X;i++)
#define U_SEC(X) (F_CPU/1000000*(X)-1)


#define SDA 1
#define SCL 2
#define BTN 3
#define TXD 6
#define RXD 7




#define F_SCL 400000UL
#define TWI_BAUD(F_SCL) (((F_CPU/F_SCL)-10)/2)

static void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}// TCB0

void TWI_init(){
	TWI0.MBAUD=TWI_BAUD(F_SCL);
	TWI0.MCTRLA=TWI_ENABLE_bm;
	TWI0.MSTATUS=TWI_BUSSTATE_IDLE_gc;
}
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
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,0);//CLKCTRL_PDIV_12X_gc|CLKCTRL_PEN_bm);

	TCB0.CTRLA=TCB_ENABLE_bm;
	TCB0.CCMP=U_SEC(1000);

	PORTA.DIRSET=_BV(SDA)|_BV(SCL)|_BV(TXD);

	TWI_init();
	_delay_ms(1000);
	while(1){
		TWI_begin(0x12);
		TWI_write(rand());
		TWI_end();
		FOR(10)wait();
	}
}
