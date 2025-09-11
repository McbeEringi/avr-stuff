#!/bin/env -S bun
import{c2seg}from'../util.mjs';
console.log(`${[...Bun.argv[2]].map(c2seg)}`);
