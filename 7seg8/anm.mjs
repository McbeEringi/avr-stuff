#!/usr/bin/bun
import{c2seg,a2b}from'./util.mjs';

const
s=[
	...`        HELLO! THIS IS A TEST PARAGRAPH FOR 7SEG DISPLAY. IT USES ONLY CHARS THAT CAN BE DISPLAYED. A B C D E F G H I J L N O P Q R S T U Y 0 1 2 3 4 5 6 7 8 9 . - _ ^ = ( ) / \\ ? ! . INPUT = UART or I2C. PROCESSOR = ATtiny202. 74HC595 RC FILTER ONE-CABLE CONNECTION. 1000us/DIGIT. 125FPS. `,
	...`        S    P    A    C    E `,
].map((x,i,a)=>({seg:c2seg(x),bri:a[i-1]==' '?1:0})),
w=Bun.file('/dev/ttyUSB0').writer();

let i=0;
setInterval(_=>(
	w.write(a2b(s.slice(i,10*8+i++))), 
	w.flush(),
	s.length<i&&(i=0)
),200);
