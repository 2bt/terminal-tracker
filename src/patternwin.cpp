#include <curses.h>
#include <ctype.h>
#include <string.h>

#include "server.h"
#include "messagewin.h"
#include "patternwin.h"
#include "styles.h"


enum {
	KEY_CTRL_DOWN = 525,
	KEY_CTRL_UP = 566,
	KEY_CTRL_RIGHT = 560,
	KEY_CTRL_LEFT = 545,
	KEY_CTRL_X = 24,
	KEY_CTRL_Y = 25,
	KEY_CTRL_N = 14,
};


void PatternWin::resize() {
	width = COLS - left;
	height = LINES - top - 4;
	scroll_x_view = (width - 4) / (CHAN_CHAR_WIDTH + 1);

	scroll_y0_view = 8;
	scroll_y1_view = height - scroll_y0_view - 5;

	do_scroll();
}


void PatternWin::draw() {
	int server_row = server.get_row();
	int server_block = server.get_block();


	set_style(FRAME);
	mvprintw(top, left, "   ");
	addch(ACS_ULCORNER);
	mvprintw(top + 1, left, "   ");
	addch(ACS_VLINE);
	move(top + 2, left);
	addch(ACS_ULCORNER);
	addchs(ACS_HLINE, 2);
	addch(ACS_PLUS);
	for (int r = 0; r < scroll_y0_view; r++) {
		move(top + r + 3, left);
		addch(ACS_VLINE);
		if (r + scroll_y0 < (int) tune.table.size()) {
			set_style(r == cursor_y0 ? HL_NORMAL : NORMAL);
			printw("%02X", r + scroll_y0);
			set_style(FRAME);
		}
		else printw("  ");
		set_style(FRAME);
		addch(ACS_VLINE);
	}



	auto& line = tune.table[cursor_y0];
	int max_rows = get_max_rows(line, tune.patterns);
	int y1 = top + scroll_y0_view + 3;

	move(y1, left);
	addch(ACS_LTEE);
	addchs(ACS_HLINE, 2);
	addch(ACS_PLUS);
	for (int r = 0; r < scroll_y1_view; r++) {
		int i = r + scroll_y1;
		move(y1 + r + 1, left);
		addch(ACS_VLINE);
		if (i < max_rows) {
			set_style(i == cursor_y1 ? HL_NORMAL : NORMAL);
			printw("%02X", i);
			set_style(FRAME);
		}
		else printw("  ");
		set_style(FRAME);
		addch(ACS_VLINE);
	}

	move(top + height - 1, left);
	addch(ACS_LLCORNER);
	addchs(ACS_HLINE, 2);
	addch(ACS_BTEE);


	int x = left + 4;
	int chan_limit = std::min(scroll_x + scroll_x_view, (int) CHANNEL_COUNT);
	for (int chan_nr = scroll_x; chan_nr < chan_limit; chan_nr++, x += CHAN_CHAR_WIDTH + 1) {

		auto& pat_name = line[chan_nr];
		auto pat = tune.patterns.count(pat_name) ? &tune.patterns[pat_name] : nullptr;


		// table head

		move(top, x);
		addchs(ACS_HLINE, CHAN_CHAR_WIDTH);
		addch(chan_nr < chan_limit - 1 ? ACS_TTEE : ACS_URCORNER);
		move(top + 1, x);

		int level = clamp(server.get_chan_level(chan_nr), 0.0f, 1.0f) * (CHAN_CHAR_WIDTH - 1);
		set_style(LEVEL);
		addchs(' ', level);
		set_style(NORMAL);
		addchs(' ', CHAN_CHAR_WIDTH - 1 - level);
		printw("%X", chan_nr);
		set_style(FRAME);
		addch(ACS_VLINE);
		move(top + 2, x);
		addchs(ACS_HLINE, CHAN_CHAR_WIDTH);
		addch(chan_nr < chan_limit - 1 ? ACS_PLUS : ACS_RTEE);


		move(y1, x);
		addchs(ACS_HLINE, CHAN_CHAR_WIDTH);
		addch(chan_nr < chan_limit - 1 ? ACS_PLUS : ACS_RTEE);

		// top

		for (int r = 0; r < scroll_y0_view; r++) {
			int i = r + scroll_y0;
			move(top + r + 3, x);
			if (i < (int) tune.table.size()) {

				int style = MACRO;
				if (i == server_block) style = PL_MACRO;
				if (i == cursor_y0) style = HL_MACRO;
				if (i == cursor_y0 && cursor_x == chan_nr) style = edit_name ? EDIT : CS_MACRO;
				set_style(style);

				auto pn = tune.table[i][chan_nr];
				printw("%s", pn.c_str());
				addchs(pn == "" ? ' ' : '.', CHAN_CHAR_WIDTH - pn.size());
			}
			else {
				set_style(FRAME);
				addchs(' ', CHAN_CHAR_WIDTH);
			}
			set_style(FRAME);
			addch(ACS_VLINE);
		}

		// bottom
		for (int r = 0; r < scroll_y1_view; r++) {
			int i = r + scroll_y1;
			move(y1 + r + 1, x);

			if (pat && i < (int) pat->size()) {
				auto& row = pat->at(i);

				int style = NOTE;
				if (i == server_row && tune.table[server_block][chan_nr] == pat_name) style = PL_NOTE;
				if (i == cursor_y1) style = HL_NOTE;
				if (i == cursor_y1 && cursor_x == chan_nr) style = CS_NOTE;
				set_style(style);


				if (row.note > 0) {
					printw("%c%c%X",
						"CCDDEFFGGAAB"[(row.note - 1) % 12],
						"-#-#--#-#-#-"[(row.note - 1) % 12],
						(row.note - 1) / 12);
				}
				else if (row.note == -1) printw("===");
				else printw("...");

				set_style(style + 1);

				for (int m = 0; m < MACROS_PER_ROW; m++) {
					std::string macro = row.macros[m];
					printw(" %s", macro.c_str());
					addchs('.', MACRO_CHAR_WIDTH - macro.size());
				}
			}
			else {
				set_style(i == cursor_y1 ? cursor_x == chan_nr ? CS_NOTE : HL_NOTE : FRAME);
				addchs(' ', CHAN_CHAR_WIDTH);
			}
			set_style(FRAME);
			addch(ACS_VLINE);
		}

		set_style(FRAME);
		move(top + height - 1, x);
		addchs(ACS_HLINE, CHAN_CHAR_WIDTH);
		addch(chan_nr < chan_limit - 1 ? ACS_BTEE : ACS_LRCORNER);

	}
	set_style(DEFAULT);
	for (int r = 0; r < height; r++) {
		move(top + r, x);
		addchs(' ', std::max(0, width - x));
	}


	if (edit_name) {
		curs_set(1);
		move(4 + cursor_y0 - scroll_y0,
			(cursor_x - scroll_x) * (CHAN_CHAR_WIDTH + 1) + 4 + line[cursor_x].size());
	}
	else curs_set(0);
}


void PatternWin::key(int ch) {
	auto& line = tune.table[cursor_y0];
	auto& pat_name = line[cursor_x];
	auto pat = tune.patterns.count(pat_name) ? &tune.patterns[pat_name] : nullptr;
	auto row = (pat && cursor_y1 < (int) pat->size()) ? &pat->at(cursor_y1) : nullptr;

	int max_rows = get_max_rows(line, tune.patterns);


	// edit pattern name
	if (edit_name) {
		if ((isalnum(ch) || ispunct(ch)) && pat_name.size() < PATTERN_CHAR_WIDTH) {
			pat_name += ch;
		}
		else if (ch == KEY_BACKSPACE && pat_name.size() > 0) pat_name.pop_back();
		else if (ch == 27) {
			edit_name = false;
			pat_name.assign(old_name);
		}
		else if (ch == '\n') {
			edit_name = false;

			// new pattern
			if (old_name == "" && pat_name != "" && tune.patterns.count(pat_name) == 0) {
				tune.patterns[pat_name].resize(16);
			}

			// TODO: delete pattern, if unused
			if (old_name != "" && pat_name != "") {

			}
		}
		return;
	}


	switch (ch) {
	case KEY_UP:
		if (--cursor_y1 < 0) cursor_y1 = std::max(0, max_rows - 1);
		do_scroll();
		return;

	case KEY_DOWN:
		if (++cursor_y1 >= max_rows) cursor_y1 = 0;
		do_scroll();
		return;

	case KEY_PPAGE:
		if ((cursor_y1 -= 4) < 0) cursor_y1 = std::max(0, cursor_y1 + max_rows);
		do_scroll();
		return;
	case KEY_NPAGE:
		if ((cursor_y1 += 4) >= max_rows) cursor_y1 = std::max(0, cursor_y1 - max_rows);
		do_scroll();
		return;

	case KEY_CTRL_DOWN:
		if (++cursor_y0 >= (int) tune.table.size()) cursor_y0 = 0;
		do_scroll();
		return;

	case KEY_CTRL_UP:
		if (--cursor_y0 < 0) cursor_y0 = std::max(0, (int) tune.table.size() - 1);
		do_scroll();
		return;


	case KEY_CTRL_RIGHT:
	case KEY_RIGHT:
		if (++cursor_x >= CHANNEL_COUNT) cursor_x = 0;
		do_scroll();
		return;

	case KEY_LEFT:
	case KEY_CTRL_LEFT:
		if (--cursor_x < 0) cursor_x = CHANNEL_COUNT - 1;
		do_scroll();
		return;

	case 'X':
		if (pat) pat->push_back({});
		return;
	case 'Y':
		if (pat && pat->size() > 0) {
			pat->pop_back();
			cursor_y1 = std::max(0, std::min(cursor_y1, (int) pat->size() - 1));
		}
		return;

	case KEY_CTRL_X:
		tune.table.push_back({});
		return;
	case KEY_CTRL_Y:
		if (tune.table.size() > 1) {
			tune.table.pop_back();
			cursor_y0 = std::min(cursor_y0, (int) tune.table.size() - 1);
		}
		return;

	case KEY_CTRL_N:
		edit_name = true;
		old_name = pat_name;
		return;

	case KEY_BACKSPACE:
		if (row) {
			row->note = 0;
			for (auto& m : row->macros) m = "";
		}
		return;

	case '<':
		if(--octave < 0) octave = 0;
		break;
	case '>':
		if (++octave > 8) octave = 8;
		break;


	default: break;
	}


	if (!row) return;


	// write note
	if (ch < 32 || ch > 127) return;
	if (ch == '^') {
		row->note = -1;
		return;
	}
	static const char* t1 = "ysxdcvgbhnjm,";
	static const char* t2 = "q2w3er5t6z7ui";
	const char* a = nullptr;
	int n;
	if ((a = strchr(t1, ch))) n = a - t1;
	else if ((a = strchr(t2, ch))) n = a - t2 + 12;
	if (a) {
		row->note = n + 1 + octave * 12;
		row->macros[0] = macro;
	}
}

void PatternWin::do_scroll() {
	if (scroll_x > cursor_x) scroll_x = cursor_x;
	if (scroll_x < cursor_x - scroll_x_view + 1) {
		scroll_x = cursor_x - scroll_x_view + 1;
	}

	if (scroll_y0 > cursor_y0) scroll_y0 = cursor_y0;
	if (scroll_y0 < cursor_y0 - scroll_y0_view + 1) {
		scroll_y0 = cursor_y0 - scroll_y0_view + 1;
	}

	if (scroll_y1 > cursor_y1) scroll_y1 = cursor_y1;
	if (scroll_y1 < cursor_y1 - scroll_y1_view + 1) {
		scroll_y1 = cursor_y1 - scroll_y1_view + 1;
	}

}
