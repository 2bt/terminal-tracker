frames = 700
ticks = | 10 6

MACRO boom
	panning = 0
	volume = 1 1.5
	sustain = 1 1 1 1 1 1 0
	decay = 0.17
	attack = 0.0023
	wave = 4 1
	pulsewidth = 0.5
	offset = 60 14 10 5 1 -2 0 -1 -2
	filter = 1
	cutoff = 1.8

MACRO pik
	panning = 0.1
	sustain = 0
	decay = 0.07
	wave = 3 0
	pulsewidth = 0.5

	filter = 0
	volume = 0.9

MACRO pik_ < pik
	panning = -0.1
	volume = 0.6


MACRO kick
	panning = -0.4
	volume = 1 1.5
	sustain = 1 1 1 1 1 1 0
	decay = 0.2
	attack = 0.0023
	wave = 4 1
	resolution = 5
	pulsewidth = 0.5
	offset = 60 12 8 3 0 -2 0 -1 -2
	filter = 3
	cutoff = 60 30 10 9 8 7 6

MACRO kick2 < kick
	panning = 0.4

MACRO snare
	filter = 1
	cutoff = 80
	panning = -0.05
	volume = 1
	sustain = 1 1 0
	decay = 0.5
	attack = 0.002
	wave = 4 0 0 4
	puslewidth = 0.5
	offset = 40 12 0 70


MACRO sn_ < snare
	panning = 0.1
	volume = 0.5

MACRO sn2 < snare
	cutoff = 60
	panning = 0.2
	volume = 1.2
	pulsewidth = 0.5
	wave = 3 2 3
	offset = 0 -2 60
	decay = 0.35

MACRO sn2_ < sn2
	panning = 0.4
	volume = 0.7


MACRO hat
	panning = 0.2
	attack = 0.0045
	sustain = 0
	decay = 0.4
	offset = 50
	wave = 4
	volume = 0.6
	cutoff = 50
	filter = 6

MACRO sup
	panning = 0.2
	offset = 40 | +-7
	pulsewidth = 0.5
	wave = 2
	filter = 5
	resolution = 3
	volume = 1
	sustain = 0
	decay = 0.35


MACRO bass
	panning = -0.1
	wave = 0
	sustain = 0.5
	pulsewidth = 0.5 0.4 0.3 0.25
	volume = 1.1

	filter = 3
	cutoff = 5 4 3 2

	decay = 0.35
	release = 0.1

MACRO pad
	volume = 0.7
	wave	= 1
	sustain = 0.4
	attack	= 0.02
	decay	= 0.17
	release = 0.17
	resolution = 0
	pulsewidth = 0 | +0.015


MACRO pad_ < pad
	volume	= 1.2
	decay	= 0.17
	sustain = 0.1

MACRO pan_a
	panning = -0.4
MACRO pan_b
	panning = 0.2
MACRO pan_c
	panning = -0.2
MACRO pan_d
	panning = 0.4

MACRO vsD
	volume = | +-0.007
MACRO vsd
	volume = | +-0.004
MACRO vsu
	volume = | +0.01



MACRO bell
	panning = 0
	volume = 1
	attack = 0.0045
	sustain = 0.2
	decay = 1.5
	release = 4
	pulsewidth = 0.2 0.3 0.4 0.5
	wave = 0
	offset = 0
	vibratodepth = 0 0 0 0 0 0 0.1
	vibratospeed = 0.07


MACRO trmp
	panning = 0
	volume = 0.9
	attack = 0.0045
	sustain = 0.2
	decay = 2
	pulsewidth = 0.1 | +0.008
	wave = 0
	offset = 0
	vibratodepth = 0 0 0 0 0 0 0.1
	vibratospeed = 0.07


