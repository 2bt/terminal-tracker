#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>

#include "tune.h"
#include "messagewin.h"
#include "patternwin.h"
#include "styles.h"

#include "server.h"




static Tune tune = { {}, {}, {}, 800, 8, };

Server		server;
PatternWin	pat_win;
MessageWin	msg_win;



void done() { endwin(); }

int main(int argc, char** argv) {
	bool write_tune = false;
START:
	if (argc < 2 || argc > 4) {
		printf("Usage: %s [-w] tune-file [tune-watch-file]\n", argv[0]);
		return 0;
	}
	if (strcmp(argv[1], "-w") == 0) {
		write_tune = true;
		if (--argc < 2) goto START;
		argv++;
	}

	const char* tunefile = argv[1];
	const char* tunewatchfile = (argc == 3) ? argv[2] : nullptr;
	int inotify_fd = 0;
	int tunewatch = 0;

	if (!load_tune(tune, tunefile)) {
		if (write_tune) {
			printf("Error loading tune file\n");
			return 1;
		}
		msg_win.say("Error loading tune file");
	}

	if (tunewatchfile) {
		if (!load_tune(tune, tunewatchfile)) {
			if (write_tune) {
				printf("Error loading tune watch file\n");
				return 1;
			}
			msg_win.say("Error loading tune watch file");
		}
		inotify_fd = inotify_init1(IN_NONBLOCK);
		tunewatch = inotify_add_watch(inotify_fd, tunewatchfile, IN_MODIFY);
	}

	if (tune.table.empty()) tune.table.resize(1);

	server.init(&tune, [&](int t, int v) { pat_win.midi(t, v); });

	if (write_tune) {
		printf("Writing tune...\n");
		server.generate_full_log();
		return 0;
	}


	atexit(&done);

	initscr();
	start_color();
	cbreak();
	noecho();
	timeout(83);
	keypad(stdscr, TRUE);
	meta(stdscr, TRUE);

	init_styles();


	pat_win.init(&tune, tunefile);
	msg_win.resize();

	server.start();

	for (;;) {

		// watch file
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

		// gui
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


		msg_win.draw();
		pat_win.draw();

		static int och = ch;
		if (ch != ERR) och = ch;
		mvprintw(LINES - 1, COLS - 24, "%3d %-20s", och, keyname(och));

		refresh();
    }
	return 0;
}


