#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#define F_SCL 400000UL
#define TWI_BAUD(X) (((F_CPU/(X))-10)/2)
#define UART_BAUD(X) (4.*F_CPU/(X)+.5)
#define FOR(X) for(uint8_t i=0;i<X;i++)
#define U_SEC(X) (F_CPU/1000000*(X)-1)

#if defined(__AVR_ATtiny202__)||defined(__AVR_ATtiny402__)
#define BTN 3
#define SET_TXD() do{PORTA.DIRSET=_BV(6);}while(0)
#elif defined(__AVR_ATtiny204__)||defined(__AVR_ATtiny404__)||defined(__AVR_ATtiny804__)||defined(__AVR_ATtiny1604__)
#define SET_TXD() do{PORTB.DIRSET=_BV(2);}while(0)
#elif defined(__AVR_ATmega808__)||defined(__AVR_ATmega809__)||defined(__AVR_ATmega1608__)||defined(__AVR_ATmega1609__)||defined(__AVR_ATmega3208__)||defined(__AVR_ATmega3209__)||defined(__AVR_ATmega4808__)||defined(__AVR_ATmega4809__)
#define SET_TXD() do{PORTA.DIRSET=_BV(0);}while(0)
#else
#error "device?"
#endif

void TWI_begin(uint8_t addr){
	TWI0.MADDR=(addr<<1)|0; // Write
	while(!(TWI0.MSTATUS&TWI_WIF_bm));
}
void TWI_write(uint8_t x){
	TWI0.MDATA=x;
	while(!(TWI0.MSTATUS&TWI_WIF_bm));
}
void TWI_end(){TWI0.MCTRLB=TWI_MCMD_STOP_gc;}

ISR(USART0_RXC_vect){
	uint8_t r=USART0.RXDATAL;
	// while(!(USART0.STATUS&USART_DREIF_bm));USART0.TXDATAL=r;
	if(TCB0.CNT==TCB0.CCMP)TWI_begin(r);
	else TWI_write(r);
	TCB0.CNT=0;
}

ISR(TCB0_INT_vect){
	TCB0.INTFLAGS=TCB_CAPT_bm;
	TWI_end();
}

int main(void) {
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,0);
	SLPCTRL.CTRLA=SLPCTRL_SMODE_IDLE_gc|SLPCTRL_SEN_bm;

	TCB0.CTRLA=TCB_ENABLE_bm;
	TCB0.CTRLB=TCB_CNTMODE_SINGLE_gc;
	TCB0.CCMP=U_SEC(2000);
	TCB0.CNT=TCB0.CCMP;
	TCB0.INTCTRL=TCB_CAPT_bm;

	USART0.BAUD=UART_BAUD(115200);
	USART0.CTRLB=USART_RXEN_bm|USART_TXEN_bm;
	USART0.CTRLA=USART_RXCIE_bm;

	TWI0.MBAUD=TWI_BAUD(F_SCL);
	TWI0.MCTRLA=TWI_ENABLE_bm;
	TWI0.MSTATUS=TWI_BUSSTATE_IDLE_gc;

	SET_TXD();
	// TWI_begin(0x12);
	// TWI_write(0xff);
	// TWI_end();
	sei();
	while(1)sleep_cpu();
}
