terminal-tracker
================

A ncurses-based chiptune tracker.
[Get a taste of what it sounds like.](https://soundcloud.com/daniel-langner-150098802/r-type-leo-area-2)

![screenshot](media/screenshot.png)

Features:
+ simple ncurses pattern editor
+ neat chiptune-like sound
+ MIDI support (now even polyphonic)
+ instruments are defined via text editor (e.g. vim)
+ effects (echo)

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
