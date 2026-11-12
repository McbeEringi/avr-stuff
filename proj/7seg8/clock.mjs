#!/usr/bin/bun
import{c2seg,a2b}from'./util.mjs';

const
w=Bun.file('/dev/ttyUSB0').writer();

setInterval(_=>(
	w.write(a2b([
		...new Date().toISOString().replace(/:/g,'-').slice(0,-1),
		...'         ',
		...new Date().toISOString().replace(/:/g,'-').slice(0,-1),
		...'         ',
	].map((x,i,a)=>({seg:c2seg(x),bri:2})),0x10)), 
	w.flush(),
),10);
