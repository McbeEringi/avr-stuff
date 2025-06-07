#!/usr/bin/bun
import{s2seg}from'./conv.mjs';

const w=[
	//...`Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.`
	...`HELLO! THIS IS A SIMPLE TEXT FOR 7SEG DISPLAY DEMO. IT USES ONLY CHARS THAT CAN BE SHOWN LIKE A B C D E F G H I L N O P S T U Y 0 1 2 3 4 5 6 7 8 9 . - _ = ? !. GOOD JOB! SYSTEM OK. UART TX RX PASS. TEST DATA 12345678. SHIFT REG READY. WAIT 500US. PUSH BTN1 OR BTN2 TO CONTINUE. LED ON. LED OFF. VOLT 3.3V. TEMP 45C. LOOP DONE. RUN NEXT STEP. CODE SAFE. KEEP COOL. DO NOT PANIC. RESET IF FAIL. INIT DONE. MODE=A. PAGE 1 OF 8. GATE OPEN. SIGNAL GOOD. SET CNT=0. DISP FULL. DEBUG END. READY? GO!`
].map(s2seg);
//await Bun.stdin.text();
console.log(w.length)
let i=0;
setInterval(_=>(
	Bun.write('/dev/ttyUSB0',new Uint8Array(w.slice(i,128+i++))),
	w.length<i&&(i=0)
),200);
