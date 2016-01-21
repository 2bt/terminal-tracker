ticks = 6
frames = 882



MACRO bass
	attack = 0
	release = 0.1
	sustain = 1
	offset = 0 60 0
	wave = 0 3 0
	filter = 1
	cutoff = 30
	pulsewidth = 0.12 | +0.01
	panning = 0.05
	volume = 1

MACRO b2 < bass
	offset = 0
	wave = 0
	volume = 0.5
	pulsewidth = 0.5
	cutoff = 2
	panning = -0.1



MACRO foo
	attack = 0
	sustain = 0.4
	decay = 0.2
	release = 0.1
	offset = 0 60 0
	wave = 0 3 0
	pulsewidth = 0.55 | +-0.01
	volume = 1 1 1 1 0.2
	filter = 3
	cutoff = 50
	panning = 0.3 -0.2



MACRO arp
	pulsewidth = 0
	wave = 1 3 1
	panning 0.2

MACRO arp1 < arp
	offset = 0 36 7 7 | 0 0 3 3 7 7

MACRO arp2 < arp
	offset = 0 36 8 8 | 0 0 5 5 8 8

MACRO arp3 < arp
	offset = 0 36 7 7 | 0 0 4 4 7 7
