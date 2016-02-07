frames = 530
ticks = | 12 10
echo_length = 20
echo_feedback = 0.3

MACRO kick
	panning = 0
	attack = 0.002
	wave = 4 0
	puslewidth = 0.5
	offset = 60 10 6 0 -4 -5
	volume = 1 1 1 | +-0.3
	filter = 3
	cutoff = 2
	decay = 2


MACRO snare < default
	panning = -0.1
	attack = 0.002
	wave = 4 0 0 4
	puslewidth = 0.5
	offset = 40 12 0 70
	volume = 1 1 1 | +-0.08
	filter = 0
	cutoff = 3

MACRO hat
	panning = 0.1
	attack = 0.002
	sustain = 0
	decay = 0.2
	wave = 3
	volume = 0.4
	filter = 0


MACRO fs
	cutoff = 1 | +0.01

MACRO fz
	cutoff = 20 | +-0.25

MACRO bass
	panning = -0.1
	wave = 0
	pulsewidth = 0.5 0.4 0.3 0.2
	volume = 1 1 1 1 0.8 0.7 0.6

#	cutoff = 30 | +-3
#	cutoff = 12
	filter = 3

MACRO pad
	wave	= 1
	attack	= 0.4535
	decay	= 10.4425
	release = 1.0442
	vibratodepth = 0.2
	vibratospeed = 0.05
	resolution = 10
	pulsewidth = 0.3


MACRO ld
	wave = 0
	offset = 0
	volume = 1
	pulsewith = 0.4
	vibratodepth = 0.15
	vibratospeed = 0.09
	echo = 0.5
