

MACRO f < default
	volume = 1
	wave = 1
	pulsewidth = 0.5
	offset = 0 | +-1.3
	release = 1.0442


MACRO x
	offset = 0
	volume = 1
	sync = 0
	wave = 0
	pulsewidth = 0.5
	ringmod = 1
	resolution = 3
	release = 1.0442

	resonance = 1
	cutoff = 3000



MACRO a
	ringmod = 0
	wave = 1
	volume = 1
	offset = 0 | +0.3

MACRO sync
	sync = 1
	wave = 1
	ringmod = 0
	volume = 1
	offset = 0 | +-0.9
	release = 0.5221



## real stuff
MACRO spdr < default
	volume = 1
	attack = 0.01
	offset = 0 10 | +-5
	pulsewidth = 0.5
	wave = 1
	filter = 6
	cutoff = 50 | +-8
	sustain = 1 1 1 0
	decay = 0.1

MACRO spdr2
	volume = 0.4
	attack = 0.003
	offset = 0 | +-4
	pulsewidth = 0.3
	wave = 0
	sustain = 1 0
	decay = 0.14


MACRO miss
	filter = 0
	volume = 0.2
	attack = 0.01
	offset = 0 35 | +-10
	pulsewidth = 0.5
	wave = 1 4
	sustain = 1 0
	decay = 0.1


MACRO blst
	attack = 0
	volume = 1
	offset = | 0 0 0 -12 -12 -24 -24 -36 -36
	pulsewidth = 0
	wave = 1
	sustain = 1 1 0
	decay = 1.5

MACRO blst_
	wave = 4
	sustain = 1 1 0
	decay = 1.5
	offset = | 0 (8|+-5)

MACRO cursr < default
	volume = 0.8
	attack = 0.01
	sustain = 1 0
	offset = -2
	decay = 0.15

MACRO slct < default
	volume = 0.8
	attack = 0.03
	sustain = (8|1) 0
	wave = 1
	resolution = 2
	resolution = 1.3 +0.3
	pulsewidth = 0.3 +0.05
	pulsewidth = 0.2
	offset = -10 (3| +0 +0 +5)
	decay = 0.23

MACRO back < default
	volume = 0.8
	attack = 0.03
	sustain = 1 1 1 1 1 0
	wave = 1
	resolution = 2 +0.1
	pulsewidth = 0.3 +0.05
	offset = -22 | +0 +0 +10 +0 +0 +0 +-10
	decay = 0.4

MACRO pause < default
	attack = 0
	offset = (5|5) (5|0) (5|5) (5|0)
	sustain = (18|1) 0
	pulsewidth = 0.4
	resolution = 2
	wave = 1
	decay = 0.2



MACRO blt
	sync = 0
	volume = 0.7
	attack = 0.01
	pulsewidth = 0.1
	resolution = 2
	wave = 4 1
	filter = 0
	sustain = 1 1 0
	decay = 0.3
	offset = 20 6 0 | +-3 +1.5




MACRO blt2
	resolution = 0
	volume = 0.7
	attack = 0.03
	pulsewidth = 0.2 | +0.1
	wave = 4 0 | 4 0 0 4 0
	sustain = 1 1 0
	decay = 0.4
	offset = 10 0 | +10 +-13 +2



MACRO rckt < default
	filter = 0
	volume = 0.4
	decay = 0
	sustain = 1
	wave = 4 4 4 4 4 4 0 0 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 0
	offset = | 0 0 12 12


MACRO spr < default
	volume = 0.4
	attack = 0
	offset = | 0 -1 -2 -3 -4 -5
	sustain = | (18|0) (18|1)
	decay = 1.4
	pulsewidth = 0.04 | (18|+0.04) (18|+-0.04)
	wave = 0

MACRO coin < default
	pulsewidth = 0.5
	volume = 0.5
	wave = 4 1
	sustain = 1 1 0
	offset = 4 2 0
	decay = 0.2


MACRO hit < default
	volume = 0.8
	wave = 1 4 1 | 1 1 1 4
	offset = 0 4  -8 -8  -17  | 0 -8  -17 -17
	sustain = 1 1 0
	decay = 0.2


# turrican 2 explosion
MACRO bm1
	wave	= |  4  0  0  4  0  4  0  4
	offset	= | 47 20 14 42 11 35 8 30
	sustain =    1  1  1  1  1  0
	decay	= 0.5

MACRO bm2
	wave	= |  4  4  0  0  0  0  0  0  0  0  0
	offset	= | 71 66 38 37 32 30 30 26 26 26 26
	sustain = (16|1) 0
	decay	= 0.8

MACRO ns2
	wave	= 4
	offset	= | 71 68 66 64 62 60 58 56 54 52 50
	sustain = 1 1 1 0
	decay	= 1


# moons enemy shoot
MACRO mns < default
	volume = 1
	attack = 0
	pulsewidth	= | (4|0.02) (4|0.5)
	wave		= | (4|0) (4|1)
	offset = 0 0.5 1 1.5 2  0.5 1.5 2.5 3.5 4.5   1 2.5 4 5.5 7  2 4 6 8 10
	sustain = 1 1 0
	decay	= 0.35


# moons item collect
MACRO coll < default
	volume = 0.4
	offset = 0 0 0 3 3 3 8 8 8 12 12 12 15 15 15 20
	wave = 0
	pulsewidth = 0.5
	sustain = (15|1) 0
	decay	= 0.2

MACRO colls
	wave = 0
	pulsewidth = 0.5
	volume = 0.3
	sync = 1
	offset = -5 | +2
	sustain = (15|1) 0
	decay	= 0.2



# moons item drop
MACRO drop < default
	volume = 0.6
	offset = 0 0 7 7 12 12 16 16 12
	wave = 0
	pulsewidth = 0.5
	sustain = 1 1 1 1 1 1 0
	decay	= 0.2


# turrican 2 laser
MACRO nl < default
	volume	= 0.2
	wave	= 4
	sustain = 1 1 0
	decay	= 1.0442
	release	= 0.3480
	offset	= 59 (12|+-3) | (4|+3) (4|+-3)

MACRO nl2 < nl
	volume	= 0
	wave	= 0


MACRO ll
	volume	= 0.3
	attack	= 0.0113
	sync	= 1
	pulsewidth = 0.1
	wave	= 0
	sustain = 1 1 0
	decay	= 1.7404
	release	= 0.3480
	offset = 64 (4| +-3) | +-1 +-1 +1

