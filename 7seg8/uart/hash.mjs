#!/usr/bin/bun
import{c2seg}from'./util.mjs';

const w=[...await Bun.file('src/main.c').bytes()].reduce((a,x,i)=>a^(x<<((i&1)&&8)),0).toString(16);
console.log(`-D SRC_HASH=0x${w} -D SRC_HASH_SEG={${[...w].map(c2seg)}}`);
