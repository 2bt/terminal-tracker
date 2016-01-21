MACRO b
	pulsewidth = 0.2
	pulsewidthsweep = 1
	offset = 0
	gliss = 0
	filter = 1


MACRO p3
	volume = 0.7
	panning = -0.2



MACRO bg < b
	gliss = 1


MACRO fs < b
	cutoff = 1

MACRO fu
	cutoff = | +0.2


MACRO fa
	cutoff = 7 | +-0.1

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


MACRO pad
	offset = | 0 0 7 7 15 15 12 12 3 3
	volume = 0.7
	sustain = 0.1
	attack = 1
	decay = 8
	release = 3
	filter = 4
	cutoff = 100 | +-0.7

MACRO p1
	panning = 0.1
MACRO p2
	panning = -0.1
