#!/usr/bin/bun
import{s2seg}from'./s2seg.mjs';

const s=[
	...`        HELLO! THIS IS A TEST PARAGRAPH FOR 7SEG DISPLAY. IT USES ONLY CHARS THAT CAN BE DISPLAYED. A B C D E F G H I J L N O P Q R S T U Y 0 1 2 3 4 5 6 7 8 9 . - _ ^ = ( ) / \\ ? ! . INPUT = UART or I2C. PROCESSOR = ATtiny402. 74HC595 RC FILTER ONE-CABLE CONNECTION. 500us/DIGIT. 250FPS. `,
	...`        S    P    A    C    E `,
].map(s2seg),
w=Bun.file('/dev/ttyUSB0').writer();

let i=0;
setInterval(_=>(
	w.write(new Uint8Array(s.slice(i,64+i++))),
	w.flush(),
	s.length<i&&(i=0)
),200);
