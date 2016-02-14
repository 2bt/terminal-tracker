#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <uv.h>
#include "tune.h"
#include "messagewin.h"
#include "patternwin.h"
#include "styles.h"
#include "server.h"


static Tune tune = { {}, {}, {}, 800, 8, };

Server		server;
PatternWin	pat_win;
MessageWin	msg_win;



const char* watch;
void reload(uv_fs_event_t* handle, const char* name, int events, int status) {
	if (events & UV_CHANGE) {

		uv_fs_event_stop(handle);

		msg_win.say("Reloading tune watch file... ");
		if (!load_tune(tune, watch)) msg_win.append("error.");
		else msg_win.append("done.");

		uv_fs_event_start(handle, &reload, watch, 0);
	}
}


void done() { endwin(); }

int main(int argc, char** argv) {
	bool write_tune = false;
START:
	if (argc < 2 || argc > 4) {
		printf("Usage: %s [-w] tune.x [tune.y]\n", argv[0]);
		printf("  -w    write entire tune to log.wav and quit\n");
		return 0;
	}
	if (strcmp(argv[1], "-w") == 0) {
		write_tune = true;
		if (--argc < 2) goto START;
		argv++;
	}

	const char* filename = argv[1];
	if (argc == 3) watch = argv[2];


	if (!load_tune(tune, filename)) {
		if (write_tune) {
			printf("Error loading tune file\n");
			return 1;
		}
		msg_win.say("Error loading tune file");
	}

	uv_loop_t* loop = uv_default_loop();
	uv_fs_event_t handle;
	if (watch) {
		if (!load_tune(tune, watch)) {
			if (write_tune) {
				printf("Error loading tune watch file\n");
				return 1;
			}
			msg_win.say("Error loading tune watch file");
		}
		uv_fs_event_init(loop, &handle);
		uv_fs_event_start(&handle, &reload, watch, 0);
	}

	if (tune.table.empty()) tune.table.resize(1);

	server.init(&tune, [&](int t, int v) { pat_win.midi(t, v); });
	if (write_tune) {
		printf("Writing tune...\n");
		server.generate_full_log();
		return 0;
	}
	server.start();


	initscr();
	start_color();
	cbreak();
	noecho();
	timeout(83);
	keypad(stdscr, TRUE);
	meta(stdscr, TRUE);
	init_styles();
	atexit(&done);

	pat_win.init(&tune, filename);
	msg_win.resize();

	for (;;) {
		uv_run(loop, UV_RUN_NOWAIT);

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

	uv_loop_close(loop);
}


