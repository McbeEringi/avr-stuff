#!/usr/bin/bun
console.log('-D SRC_HASH=0x'+[...await Bun.file('src/main.c').bytes()].reduce((a,x,i)=>a^(x<<((i&1)&&8)),0).toString(16));
