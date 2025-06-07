#!/usr/bin/bun
import{s2seg}from'./s2seg.mjs';

await Bun.stdout.write(
	new Uint8Array([...(await Bun.stdin.text())].slice(0,-1).map(s2seg))
);
