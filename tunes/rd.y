ticks = 8
frames = 650

MACRO kick
	wave = 4 0
	puslewidth = 0.5
	offset = 60 10 6 0 -4 -5
	volume = 1 1 1 1 | +-0.3
	decay = 1

MACRO snare
	wave = 4 0 0 0 4
	puslewidth = 0.5
	offset = 40 12 4 0 70
	volume = 1 1 1 1 1 | +-0.08
	decay = 1

MACRO tick
	volume = 0.6
	wave = 4
	offset = 30
	sustain = 0
	decay = 0.1


MACRO bass
	wave = 4 1
	offset = 20 0
#	resolution = 5
	pulsewidth = 0
	filter = 3
	cutoff = 8 | +-0.3
	volume = 1.2
	decay = 0.5
	susain = 0.4

MACRO lead
	offset = 40 0
	wave = 4 0
	pulsewidth = 0.1
	pulsewidthsweep = 0.5

	vibratodepth = (20|0) 0.3
	vibratospeed = 0.1


MACRO pad
	decay = 1
	sustain = 0.4

MACRO pad1 < pad
	offset = | 0 0 3 3 7 7 10 10

MACRO pad2 < pad
	offset = | 0 0 2 2 4 4 7 7


MACRO crd
	attack = 2
	decay = 2
	release = 2
	wave = 1
	pulsewidth = 0.5

MACRO crd1 < crd
	offset = | 0 0 4 4 7 7 11 11 14 14

MACRO crd2 < crd
	offset = | 0 0 3 3 7 7 10 10 14 14


MACRO crd3 < crd
	offset = | -2 -2 2 2 5 5 9 9 12 12

