ticks = 6
frames = 840

MACRO kick
	attack = 0.0023
	wave = 4 0
	puslewidth = 0.5
	offset = 60 10 6 0 -4 -5
	volume = 1 1 1 | +-0.3
	resonance = 0


MACRO snare
	attack = 0.0023
	wave = 4 0 0 4
	puslewidth = 0.5
	offset = 40 12 0 70
	volume = 1 1 1 | +-0.08
	resonance = 2
	cutoff = 6000

MACRO bass
	panning = -0.1 -0.1 -0.1 -0.1 0.2
	wave = 0
	pulsewidth = 0.5 0.4 0.3 0.2
	volume = 1 1 1 1 0.8 0.7 0.6
	cutoff = 5000 +-250 +-250 +-250 +-250 +-250 +-250 +-250 +-250 +-250 +-250 +-250 +-250 +-250 +-250 +-250 +-250 +-250
	resonance = 1


MACRO kb
	volume = 0.8
	attack	= 0.0227
	sustain	= 0.3
	decay	= 1.3053
	release	= 1.3053
	wave = 0
	resolution = 8
	pulsewidth = 0.6 +0.03 +0.03 +0.03 +0.03 +0.03 +0.03 +0.03 +0.03 +0.03 +0.03
	vibratodepth = 0.1
	vibratospeed = 0.08


MACRO midi < kb
