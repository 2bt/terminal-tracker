terminal-tracker
================

A ncurses-based chiptune tracker.
The tracker comprises a pattern editor only.
Instruments (which for some reason are refered to as macros) must be defined via text editor (e.g. vim).
[Get a taste of what it sounds like.](https://soundcloud.com/daniel-langner-150098802/r-type-leo-area-2)
Basic MIDI input is supported.
Unfortunately, there is no documentation besides the source code itself.

Libs you'll need:
- `libncurses`
- `libSDL`
- `libportmidi`
- `libsndfile`

Try it out:

	$ git submodule init
	$ make
	$ ./tt tunes/area2.x	# press space to play
