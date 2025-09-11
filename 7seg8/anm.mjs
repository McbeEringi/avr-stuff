#!/usr/bin/bun
import{c2seg,a2b}from'./util.mjs';

const
w=Bun.file('/dev/ttyUSB0').writer();

let i=0;
setInterval(s=>(
	s=[
		// ...` HELLO! THIS IS A TEST PARAGRAPH FOR 7SEG DISPLAY. IT USES ONLY CHARS THAT CAN BE DISPLAYED. A B C D E F G H I J L N O P Q R S T U Y 0 1 2 3 4 5 6 7 8 9 . - _ ^ = ( ) / \\ ? ! . INPUT = UART or I2C. PROCESSOR = ATtiny202. 74HC595 RC FILTER ONE-CABLE CONNECTION. 1000us/DIGIT. 125FPS. `,
		// ...`fragile!! do not touch !!!`
		// ...`Lunch...    Lunch...    Lunch...    Lunch...    `
		...`UEC BOOTH    UEC BOOTH    UEC BOOTH    `
		//...`        S    P    A    C    E `,
		//...`5000000000000000 YEN HOSII!!!    `,
		//...new Date().toISOString().replace(/:/g,"'").slice(0,-1)+'        '
	].map((x,i,a)=>({seg:c2seg(x),bri:!i||a[i-1]==' '?3:2})),
	w.write(a2b(s.concat(s).slice(i,8*8+i++),0x10)), 
	w.flush(),
	i<s.length||(i=0)
),200);
