#include <stdlib.h>
#include <string.h>
#include <getopt.h>
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


void usage(const char* prg) {
	fprintf(stderr, "Usage: %s [options] tune.x [tune.y]\n", prg);
	fprintf(stderr, "  -w         Write entire tune to log.wav and quit.\n");
	fprintf(stderr, "  -r reps    Repeate tune. Implies -w.\n");
	fprintf(stderr, "  -s nr      Choose subtune. Implies -w.\n");
}


int main(int argc, char** argv) {
	bool write_tune = false;
	int write_reps = 1;
	int write_subtune = 0;

	int opt;
	while ((opt = getopt(argc, argv, "wr:s:")) != -1) {
		switch (opt) {
		case 'w':
			write_tune = true;
			break;
		case 'r':
			write_tune = true;
			write_reps = atoi(optarg);
			break;
		case 's':
			write_tune = true;
			write_subtune = atoi(optarg);
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}
	int opts_left = argc - optind;
	if (opts_left < 1 || opts_left > 2) {
		usage(argv[0]);
		return opts_left != 0;
	}

	const char* filename = argv[optind];
	if (opts_left == 2) watch = argv[optind + 1];


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
		server.generate_full_log(write_subtune, write_reps);
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


