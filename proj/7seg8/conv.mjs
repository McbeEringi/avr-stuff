#!/usr/bin/bun
import{c2seg,a2b}from'./util.mjs';

const w=Bun.file('/dev/ttyUSB0').writer();
await new Promise(f=>setTimeout(f,100));
w.write(a2b([...await Bun.stdin.text()].slice(0,-1).map(x=>({seg:c2seg(x),bri:1}))));
w.flush();
