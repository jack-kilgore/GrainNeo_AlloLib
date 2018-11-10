s.reboot;
b = Buffer.read(s,"D:/SketchFace2.0/REAL FINAL for SketchFaces cutshort/overall1.5 condensed as fuck.wav");
c = Buffer.read(s,"C:/Users/Jackalaka/Desktop/misc samples/feedback_atWork (9_28_18)/TASCAM_0730.wav");
d = Buffer.read(s,"C:/Users/Jackalaka/Desktop/misc samples/10_9_18/beautiful_birds_clean.wav" );
s.reboot;
(
~set1013 = Array.new;
~folder13 = PathName.new("C:/Users/Jackalaka/Desktop/misc samples/10_13 set");
(
~folder13.entries.do({
	arg path;
	~set1013 = ~set1013.add(Buffer.read(s,
		path.fullPath));
});
)
)

(
~scsamples = Array.new;
~folder2 = PathName.new("C:/Users/Jackalaka/Desktop/supercollider samples/scdir for sc samples" );
(
~folder2.entries.do({
	arg path;
	~scsamples = ~scsamples.add(Buffer.read(s,
		path.fullPath));
});
)
)

(
~conserve_sound = Array.new;
~folder12 = PathName.new("C:/Users/Jackalaka/Desktop/misc samples/conserve_sound");
(
~folder12.entries.do({
	arg path;
	~conserve_sound = ~conserve_sound.add(Buffer.read(s,
		path.fullPath));
});
)
)

(
~chorus= Array.new;
~folder100 = PathName.new("C:/Users/Jackalaka/Desktop/misc samples/chorus/10770__klankbeeld__chorus");
(
~folder100.entries.do({
	arg path;
	~chorus = ~chorus.add(Buffer.read(s,path.fullPath));
})
))

(
~ivwalks = Array.new;
~folder1 = PathName.new("C:/Users/Jackalaka/Desktop/misc samples/walk around isla vista sat night/tascamsqueek");
(
~folder1.entries.do({
	arg path;
	~ivwalks = ~ivwalks.add(Buffer.read(s,
		path.fullPath));
});
)
)

(
~bowlspoon = Array.new;
~folder9 = PathName.new("C:/Users/Jackalaka/Desktop/misc samples/bowl and spoon");
(
~folder9.entries.do({
	arg path;
	~bowlspoon = ~bowlspoon.add(Buffer.read(s,path.fullPath));
})
))

(
~dusk= Array.new;
~folder6 = PathName.new("C:/Users/Jackalaka/Desktop/misc samples/9_8_18dusk_walk/scdir");
(
~folder6.entries.do({
	arg path;
	~dusk = ~dusk.add(Buffer.read(s,path.fullPath));
})
))

(
~pan_kitchen= Array.new;
~folder101 = PathName.new("C:/Users/Jackalaka/Desktop/misc samples/pan_line/pan_line_conserve_sound");
(
~folder101.entries.do({
	arg path;
	~pan_kitchen = ~pan_kitchen.add(Buffer.read(s,path.fullPath));
})
))

(

( //a gabor(approx. gaussian-shaped) grain
SynthDef(\gabor1,{|out, amp=0.01, freq=440, sustain=0.01, pan, bufnum, trigs=2000, rate =1,
	samp_rand_rate = 1, samp_rand_range = 2|
	var snd = FSinOsc.ar(freq);
	var trig =  Blip.kr(trigs,1,80);
	var sig = PlayBuf.ar(2, bufnum, BufRateScale.kr(bufnum)*MouseY.kr(1.1,2.1)*rate, trig,
		LFNoise1.kr(samp_rand_rate).range(1,samp_rand_range)*MouseX.kr(0, BufFrames.kr(bufnum)));
	var amp2 = amp*AmpComp.ir(freq.max(50))*0.5;
	var env = EnvGen.ar(Env.sine(sustain,amp2),
		doneAction:2);
	OffsetOut.ar(out,HPF.ar(Pan2.ar(sig*env,pan),25));
},\ir!5).add;
);

(//wider,quasi-gauss env, w/hold time in middle
SynthDef(\gabWide1,{|out, amp=0.01, freq=440, sustain=0.01, pan, width=0.5, bufnum, trigs=2000, rate =1,
	samp_rand_rate = 1, samp_rand_range = 2|
	var holdT = sustain*width;
	var fadeT = width*sustain*0.5;
	var snd = FSinOsc.ar(freq);
	var trig =  Blip.kr(trigs,1,80);
	var sig = PlayBuf.ar(2, bufnum, BufRateScale.kr(bufnum)*MouseY.kr(1.1,2.1)*rate, trig,
		LFNoise1.kr(samp_rand_rate).range(1,samp_rand_range)*MouseX.kr(0, BufFrames.kr(bufnum)));
	var amp2 = amp*AmpComp.ir(freq.max(50))*0.4;
	var env = EnvGen.ar(
		Env([0,1,1,0],[fadeT,holdT,fadeT],\sin),
		levelScale:amp2,
		doneAction:2);
	OffsetOut.ar(out,HPF.ar(Pan2.ar(sig*env,pan),25));
},\ir!5).add;
);

(//percussive envelope
SynthDef(\percSin1,{|out,amp=0.1, freq=440, sustain=0.01, pan, bufnum, trigs=2000, rate =1,
	samp_rand_rate = 1, samp_rand_range = 2|
	var snd = FSinOsc.ar(freq);
	var trig =  Blip.kr(trigs,1,80);
	var sig = PlayBuf.ar(2, bufnum, BufRateScale.kr(bufnum)*MouseY.kr(1.1,2.1)*rate, trig,
		LFNoise1.kr(samp_rand_rate).range(1,samp_rand_range)*MouseX.kr(0, BufFrames.kr(bufnum)));
	var amp2 = amp * AmpComp.ir(freq.max(50))*0.5;
	var env = EnvGen.ar(
		Env.perc(0.1,0.9,amp2),
		timeScale: sustain,
		doneAction:2
	);
	OffsetOut.ar(out,HPF.ar(Pan2.ar(sig*env,pan),25));
},\ir!5).add;
);

(//reverse percussive env
SynthDef(\rpercSin1,{|out, amp=0.1, freq=440, sustain=0.01, pan, bufnum, trigs=2000, rate =1,
	samp_rand_rate = 1, samp_rand_range = 2|
	var snd = FSinOsc.ar(freq);
	var trig =  Blip.kr(trigs,1,80);
	var sig = PlayBuf.ar(2, bufnum, BufRateScale.kr(bufnum)*MouseY.kr(1.1,2.1)*rate, trig,
		LFNoise1.kr(samp_rand_rate).range(1,samp_rand_range)*MouseX.kr(0, BufFrames.kr(bufnum)));
	var amp2 = amp*AmpComp.ir(freq.max(50))*0.5;
	var env = EnvGen.ar(
		Env.perc(0.9, 0.1,amp2),
		timeScale:sustain,
		doneAction:2
	);
	OffsetOut.ar(out,HPF.ar(Pan2.ar(sig*env,pan),25));
},\ir!5).add;
);

(//expo decay envelope
SynthDef(\expodec1,{|out, freq=440, amp=0.3, sustain=0.01, pan, bufnum, trigs=2000, rate =1,
	samp_rand_rate = 1, samp_rand_range = 2|
	var snd = FSinOsc.ar(freq);
	var trig =  Blip.kr(trigs,1,80);
	var sig = PlayBuf.ar(2, bufnum, BufRateScale.kr(bufnum)*MouseY.kr(1.1,2.1)*rate, trig,
		LFNoise1.kr(samp_rand_rate).range(1,samp_rand_range)*MouseX.kr(0, BufFrames.kr(bufnum)));
	var amp2 = amp*AmpComp.ir(freq.max(50))*0.5;
	var env = XLine.ar(amp2,amp2*0.001,sustain,doneAction:2);
	OffsetOut.ar(out,HPF.ar(Pan2.ar(sig*env,pan),25));
},\ir!5).add;//5 refers to num arg
);

(//reverse expo decay envelope
SynthDef(\rexpodec1,{|out, freq=440, amp=0.3, sustain=0.01, pan,
	bufnum, trigs=2000, rate= 1,
	samp_rand_rate = 1, samp_rand_range = 2|
	var trig =  Blip.kr(trigs,1,80);
	var sig = PlayBuf.ar(2, bufnum, BufRateScale.kr(bufnum)*MouseY.kr(1.1,2.1)*rate, trig,
		LFNoise1.kr(samp_rand_rate).range(1,samp_rand_range)*MouseX.kr(0, BufFrames.kr(bufnum)));
	var snd = FSinOsc.ar(freq);
	var amp2 = amp*AmpComp.ir(freq.max(50))*0.5;
	var env = XLine.ar(amp2*0.001,amp2,sustain,doneAction:2)
	*(AmpComp.ir(freq)*0.5);
	OffsetOut.ar(out,HPF.ar(Pan2.ar(sig*env,pan),25));

},\ir!5).add;//5 refers to num arg
)
)
s.reboot;
//	\sustain,Pn(Pgeom(0.01,1.2,35),inf),

//dusk[5]: start out w rexpodec and go to expodec
//dusk[4]

(
Pbindef(\yes,
	\instrument,\rpercSin1,  //rpercSin1 expodec1 rexpodec1 percSin1 gabWide1 gabor1
	\sustain,Pn(Pgeom(0.1,1.2,35),inf),
	\dur,Pn(Pgeom(0.09,1.2,10),inf),
	\bufnum, d,
	\trigs, 0.001,
	\amp, 1,
	\rate, Pn(Pbrown(0.7,0.1),inf),
	\samp_rand_rate, 1,
	\samp_rand_range, 1.1

).play;
)


s.reboot;
(
	Synth.new
	(\expodec,[
		\bufnum,
	b,
		\out, 0,
	    \pan, 0
	]
	);
)

