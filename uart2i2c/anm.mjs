#!/usr/bin/bun
import{c2seg,a2b}from'./util.mjs';

const
w=Bun.file('/dev/ttyUSB0').writer(),
nt=x=>[
			x,32,
		0b00000000,0b00000000,
		0b00000000,0b00000000,
		0b00000000,0b00000000,
		0b00000000,0b00000000,
		0b00000000,0b00000000,
		0b00000000,0b00000000,
		0b00000000,0b00000000,
		0b00000000,0b00000000,

		0b00000000,0b00000000,
		0b01000001,0b01111111,
		0b01100001,0b00001000,
		0b01010001,0b00001000,
		0b01001001,0b00001000,
		0b01000101,0b00001000,
		0b01000011,0b00001000,
		0b01000001,0b00001000,
],
knzw=x=>[
			x,32,
		0b00001000,0b00000000,
		0b00010100,0b01101111,
		0b00100010,0b00001001,
		0b01011101,0b01101111,
		0b00001000,0b00001010,
		0b00111110,0b00101010,
		0b00101010,0b01001001,
		0b01111111,0b01010001,

		0b00001000,0b00000000,
		0b00010100,0b01101111,
		0b00100010,0b00001001,
		0b01011101,0b01101111,
		0b00001000,0b00001010,
		0b00111110,0b00101010,
		0b00101010,0b01001001,
		0b01111111,0b01010001,
]


let i=0;
setInterval(s=>(
	s=[
		// ...` HELLO! THIS IS A TEST PARAGRAPH FOR 7SEG DISPLAY. IT USES ONLY CHARS THAT CAN BE DISPLAYED. A B C D E F G H I J L N O P Q R S T U Y 0 1 2 3 4 5 6 7 8 9 . - _ ^ = ( ) / \\ ? ! . INPUT = UART or I2C. PROCESSOR = ATtiny202. 74HC595 RC FILTER ONE-CABLE CONNECTION. 1000us/DIGIT. 125FPS. `,
		//...`        S    P    A    C    E `,
		// ...'    ',
		...`5000000000000000 YEN HOSII!!!`,
		// ...'    ',
		// ...new Date().toISOString().replace(/:/g,"'").slice(0,-1),
		...'    ',
	].map((x,i,a)=>({seg:c2seg(x),bri:!i||a[i-1]==' '?2:1})),
	w.write(new Uint8Array([
		...a2b([...Array(Math.ceil(8*8/s.length)+1)].flatMap(_=>s).slice(++i),0x10),
		...(i&0b11?[]:[...nt((i>>>2)&1?0x21:0x20),...knzw((i>>>2)&1?0x20:0x21)])
	])), 
	w.flush(),
	i<s.length||(i=0)
),200);
