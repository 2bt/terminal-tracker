#include <curses.h>
#include <ctype.h>
#include <string.h>

#include "tune.h"
#include "messagewin.h"
#include "patternwin.h"
#include "styles.h"

#include "server.h"



static Tune tune = {
	{ { "bass" } },
	{ { "bass", { { 61, "foo" }, {}, { -1 }, {},
				  {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, } } },
	{
		{
			"default", {
				{},
				{
					{ "wave",			0 },
					{ "offset",			0 },
					{ "volume",			1 },
					{ "panning",		0 },
					{ "pulsewidth",		0.5 },
					{ "resolution",		0 },
					{ "vibratospeed",	0 },
					{ "vibratodepth",	0 },
				}
			}
		},



		{
			"foo", {
				{ "default" },
				{
					{ "wave", {{3, 0}}},
					{ "pulsewidth", {{0.3, {0.005, true}}, 1}},
					{ "vibratospeed", 0.1 },
					{ "vibratodepth", 0.4 },
//					{ "volume", 0.1 },
				}
			}
		},
	},
	8,
	800
};




Server server(tune);
PatternWin pat_win(tune);
MessageWin msg_win;


void done() { endwin(); }
int main() {

	if (!load_tune(tune, "tune")) {
		msg_win.say("error loading tune");
	}

	if (!save_tune(tune, "tune_")) {
		msg_win.say("error saving tune");
	}

	atexit(&done);

	initscr();
	start_color();
	cbreak();
	noecho();
	timeout(50);
	keypad(stdscr, TRUE);
	meta(stdscr, TRUE);

	init_styles();

	pat_win.resize();
	msg_win.resize();


	server.init();

	bool running = true;
	while (running) {
		int ch = getch();
		switch (ch) {
		case ERR: break;
		case KEY_RESIZE:
			pat_win.resize();
			msg_win.resize();
			break;

		default:
			pat_win.key(ch);
			break;
		}

		static int och = ch;
		if (ch != ERR) och = ch;
		mvprintw(0, 0, "%d %-20s", och, keyname(och));

		msg_win.draw();
		pat_win.draw();

		refresh();
    }
	return 0;
}


