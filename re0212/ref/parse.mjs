#!/usr/bin/bun
const
t=562,
b2u=w=>w.reduce((a,x,i)=>a<<1|x,0).toString(16).padStart(Math.ceil(w.length/4),0),
parse=w=>(
	w=w.match(/\d+,\d+\.\d+/g).reduce((a,x,i,_)=>(
		x=+x.split(',')[1]<1000,
		a.x!=x&&(
			a.p&&(
				_=Math.round((i-a.p)*a.t/t),
				a.x?a.a.push([_]):a.a[a.a.length-1].push(_),
				a.b.push({l:_,i:[a.p,i],x:a.x})
			),
			a.p=i,a.x=x
		),
		a
	),{
				a:[],b:[],x:1,p:0,t:+w.match(/Time interval\s*:,(\d+\.\d+)uS/)[1]
	}),
	w=w.a.slice(1,33).map(x=>+(2<x[1])),
	[
		b2u(w.slice(0,16)),
		b2u(w.slice(16,24)),
		b2u(w.slice(24,32))
	]
);

console.log(
	(
		await Promise.all(Bun.argv.slice(2).map(async x=>[x,
			x.slice(-4).toLowerCase()=='.csv'&&parse(await Bun.file(x).text())
		]))
	).reduce((a,[i,x])=>(x&&(a[i]=x),a),{})
);
