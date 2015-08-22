#include <curses.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>

#include "tune.h"
#include "messagewin.h"
#include "patternwin.h"
#include "styles.h"

#include "server.h"



static Tune tune = {
	{{}}, {},
	{
		{
			"def", {
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
	},
	800, 8,
};




Server		server;
PatternWin	pat_win;
MessageWin	msg_win;


void done() { endwin(); }
int main(int argc, char** argv) {

	if (argc != 2 && argc != 3) {
		printf("usage: %s tune-file [tune-watch-file]\n", argv[0]);
		return 0;
	}
	const char* tunefile = argv[1];
	const char* tunewatchfile = (argc == 3) ? argv[2] : nullptr;


	int inotify_fd = 0;
	int tunewatch = 0;

	if (!load_tune(tune, tunefile)) {
		msg_win.say("Error loading tune file");
	}

	if (tunewatchfile) {
		inotify_fd = inotify_init1(IN_NONBLOCK);
		tunewatch = inotify_add_watch(inotify_fd, tunewatchfile, IN_MODIFY);
		if (!load_tune(tune, tunewatchfile)) {
			msg_win.say("Error loading tune watch file");
		}
	}

//	if (!save_tune(tune, "tune_")) {
//		msg_win.say("error saving tune");
//	}

	atexit(&done);

	initscr();
	start_color();
	cbreak();
	noecho();
	timeout(50);
	keypad(stdscr, TRUE);
	meta(stdscr, TRUE);

	init_styles();


	pat_win.init(&tune, tunefile);
	msg_win.resize();

	server.init(&tune);

	bool running = true;
	while (running) {

		if (tunewatchfile) {
			inotify_event event;
			int r;
			while ((r = read(inotify_fd, &event, sizeof(inotify_event))) > 0) {

				if (event.wd != tunewatch) continue;

				msg_win.say("Reloading tune watch file... ");

				inotify_rm_watch(inotify_fd, tunewatch);
				tunewatch = inotify_add_watch(inotify_fd, tunewatchfile, IN_MODIFY);

				if (!load_tune(tune, tunewatchfile)) msg_win.append("error.");
				else msg_win.append("done.");
			}
		}


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


