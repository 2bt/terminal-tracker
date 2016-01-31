frames = 660
#ticks = 8
ticks = | 7 9


MACRO boom
	panning = 0
	volume = 1 1.5
	sustain = 1 1 1 1 1 1 0
	decay = 0.17
	attack = 0.002
	wave = 4 1
	pulsewidth = 0.5
	offset = 60 14 10 5 1 -2 0 -1 -2
	filter = 1
	cutoff = 1.8


MACRO kick
	panning = 0
	volume = 1 1.5
	sustain = 1 1 1 1 1 0
	decay = 0.2
	attack = 0.002
	wave = 4 1
	pulsewidth = 0.5
	offset = 60 12 8 3 0 -2 0 -1 -2
	filter = 1
	cutoff = 40


MACRO hat
	panning = 0.1
	sustain = 0
	decay = 0.07
	wave = 3 0
	pulsewidth = 0.5

	filter = 0
	volume = 0.9

MACRO hat_ < hat
	panning = -0.1
	volume = 0.6



MACRO sn
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


MACRO sn_ < sn
	panning = 0.1
	volume = 0.5

MACRO sn2 < sn
	cutoff = 60
	panning = 0.2
	volume = 1.2
	pulsewidth = 0.5
	wave = 3 2 3
	offset = 0 -2 60
	decay = 0.35

MACRO b
	wave = 0
	pulsewidth = 0.3
	pulsewidthsweep = 10
	release = 0.1
	filter = 1
	cutoff = 100 (150|+-0.5)
	panning = -0.1


MACRO pad
	volume = 0.4
	attack = 0.05
	sustain = 0.1
	decay = 7
	release = 0.2
	wave = 1
	pulsewidth = 0
	pulsewidthsweep = 0.5


MACRO p-a < pad
	panning = 0.1
MACRO p-b < pad
	panning = 0.3
MACRO p-c < pad
	panning = -0.3
MACRO p-d < pad
	panning = -0.1


