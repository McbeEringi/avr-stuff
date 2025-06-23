#include <avr/io.h>
#include <avr/interrupt.h>
#define U_SEC(X) (F_CPU/1000000*(X)-1)
#define FOR(X) for(uint8_t i=0;i<X;i++)

#define SERCLK 6
#define RCLK 7
#define OE_ 3
#define OE_CMP CMP0BUF
#define ADDR 0x12

const uint8_t num[]={
	0b11111100,0b01100000,0b11011010,0b11110010,
	0b01100110,0b10110110,0b10111110,0b11100000,
	0b11111110,0b11110110,0b11101110,0b00111110,
	0b10011100,0b01111010,0b10011110,0b10001110
};

const uint8_t s_hello[8]={// -HELLO!-
	2,
	0b01101110,
	0b10011110,
	0b00011100,
	0b00011100,
	0b11111100,
	0b01000001,
	2
};
#define HELLO_BRI 0x1554

// const uint8_t s_mode[4]={
// 	0b01111100,0b11101110,0b00001010,0b00011111,// UART
// 	//0,0b01100000,0b11011010,0b10011101,// I2C
// };

const uint8_t s_hash[4]={0b01101110,0b11101110,0b10110110,0b01101111};
const uint16_t src_hash=SRC_HASH;
#define HASH_BRI 0xaa55


const uint8_t bmap[]={252,224,148,0};// pow3 [.25,.5,.75,1].map(x=>(1-x**3)*256)

volatile uint8_t disp[8]={[0 ... 7]=0xff};
volatile uint16_t bri=0xe4e4;// 0b88776655'44332211
volatile uint16_t ms=0;

static void wait(){while(!(TCB0.INTFLAGS&TCB_CAPT_bm));TCB0.INTFLAGS=1;}// TCB0

volatile cnt=0;
ISR(TWI0_TWIS_vect) {
	if(TWI0.SSTATUS&TWI_APIF_bm){
		if(TWI0.SSTATUS&TWI_AP_bm){
			cnt=0;ms=-1;
			// if(TWI0.SSTATUS&TWI_DIR_bm){
			// 	// write
			// }else{
			// 	// recieve
			// }
			TWI0.SCTRLB=TWI_SCMD_RESPONSE_gc;
		}else{
			// stop
			TWI0.SCTRLB=TWI_SCMD_COMPTRANS_gc;
		}
		TWI0.SSTATUS=TWI_APIF_bm;
	}

	if(TWI0.SSTATUS&TWI_DIF_bm){
		uint8_t r=TWI0.SDATA;
		if(cnt<8)disp[cnt]=r;
		else if(cnt==8)bri=(bri&0xff00)|r;
		else if(cnt==9)bri=(bri&0x00ff)|(r<<8);

		TWI0.SCTRLB=++cnt<10?TWI_SCMD_RESPONSE_gc:TWI_SCMD_COMPTRANS_gc;
		TWI0.SSTATUS=TWI_DIF_bm;
	}
}

void main(){
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,0);//CLKCTRL_PDIV_12X_gc|CLKCTRL_PEN_bm);

	// OE# CMP0
	TCA0.SINGLE.CTRLA=TCA_SINGLE_ENABLE_bm;
	TCA0.SINGLE.CTRLB=TCA_SINGLE_CMP0EN_bm|TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	TCA0.SINGLE.PER=255;

	TCB0.CTRLA=TCB_ENABLE_bm;

	TWI0.SADDR=ADDR<<1;
	TWI0.SCTRLA=TWI_DIEN_bm|TWI_APIEN_bm|TWI_ENABLE_bm;

	PORTA.DIRSET=_BV(SERCLK)|_BV(RCLK)|_BV(OE_);
	PORTA.OUTSET=_BV(SERCLK)|_BV(RCLK);

	sei();
	while(1)for(uint8_t i=0;i<8;++i){// 1000us
		TCB0.CCMP=U_SEC(15);
		// >> ABCDEFGd 01234567
		uint16_t w=(disp[i]<<8)|((1<<(7-i))^255);
		for(uint8_t j=15;j<16;--j){// 15*2*16=480 us
			wait();
			if((w>>j)&1){PORTA.OUTCLR=_BV(SERCLK);PORTA.OUTSET=_BV(SERCLK);wait();}
			else{PORTA.OUTCLR=_BV(SERCLK);wait();PORTA.OUTSET=_BV(SERCLK);}
		}
		TCB0.CCMP=U_SEC(520);
		TCA0.SINGLE.OE_CMP=bmap[(bri>>(i*2))&0b11];
		PORTA.OUTCLR=_BV(RCLK);
		PORTA.OUTSET=_BV(RCLK);
		if(ms==1000){FOR(8)disp[i]=s_hello[i];bri=HELLO_BRI;}
		if(ms==2000){
			FOR(4){disp[i]=s_hash[i];disp[i+4]=num[(src_hash>>((3-i)*4))&0xf];}
			bri=HASH_BRI;
		}
		wait();if(~ms)++ms;
	}
}
