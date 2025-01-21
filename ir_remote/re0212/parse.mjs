#!/usr/bin/bun
const
t=562,
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
	''+[...Array(Math.ceil(w.length/8))].map((_,i)=>'0x'+parseInt(w.slice(i*8,++i*8).reverse().join(''),2).toString(16).padStart(2,0))
);

console.log(
	(
		await Promise.all(Bun.argv.slice(2).map(async x=>[x,
			x.slice(-4).toLowerCase()=='.csv'&&parse(await Bun.file(x).text())
		]))
	).reduce((a,[i,x])=>(x&&(a[i]=x),a),{})
);
