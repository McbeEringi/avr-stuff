#!/bin/bun
import{c2seg}from'../util.mjs';
const addr=+process.env.ADDR;
if(isNaN(addr))throw'env var ADDR is NaN!';

console.log(`-D I2C_ADDR=${addr} -D I2C_ADDR_SEG=\\'((uint8_t[]){${[...` ADDR ${addr.toString(16)}`].map(c2seg)}})\\' -D ADDR_BRI=0xa555`);
