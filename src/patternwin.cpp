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
	KEY_ESCAPE = 27,
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
		if (r + scroll_y0 < (int) tune->table.size()) {
			set_style(r == cursor_y0 ? HL_NORMAL : NORMAL);
			printw("%02X", r + scroll_y0);
			set_style(FRAME);
		}
		else printw("  ");
		set_style(FRAME);
		addch(ACS_VLINE);
	}



	auto& line = tune->table[cursor_y0];
	int max_rows = get_max_rows(line, tune->patterns);
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
		auto pat = tune->patterns.count(pat_name) ? &tune->patterns[pat_name] : nullptr;


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
			if (i < (int) tune->table.size()) {

				int style = MACRO;
				if (i == server_block) style = PL_MACRO;
				if (i == cursor_y0) style = HL_MACRO;
				if (i == cursor_y0 && cursor_x == chan_nr) style = (edit_mode == PATTERN) ? ET_MACRO : CS_MACRO;
				set_style(style);

				auto pn = tune->table[i][chan_nr];
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
				if (i == server_row && tune->table[server_block][chan_nr] == pat_name) style = PL_NOTE;
				if (i == cursor_y1) style = HL_NOTE;
				if (edit_mode == MARK_PATTERN && chan_nr == cursor_x) {
					if ((cursor_y1 <= i && i <= mark_y)
					||	(cursor_y1 >= i && i >= mark_y)) style = MK_NOTE;
				}
				if (i == cursor_y1 && cursor_x == chan_nr) style = (edit_mode == MACRO) ? ET_NOTE : CS_NOTE;
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


	mvprintw(top - 1, left + 20, "Oct:%d|Macro:%-8s", octave, macro.c_str());


	if (edit_mode == PATTERN) {
		curs_set(1);
		move(4 + cursor_y0 - scroll_y0,
			(cursor_x - scroll_x) * (CHAN_CHAR_WIDTH + 1) + 4 + line[cursor_x].size());
	}
	else if (edit_mode == MACRO) {
		curs_set(1);
		auto& pat_name = line[cursor_x];
		auto& pat = tune->patterns[pat_name];
		auto& row = pat[cursor_y1];
		move(4 + scroll_y0_view + cursor_y1 - scroll_y1 + 1,
			(cursor_x - scroll_x) * (CHAN_CHAR_WIDTH + 1) + 4 + row.macros[0].size() + 4);
	}
	else curs_set(0);
}

void PatternWin::move_cursor(int dx, int dy0, int dy1) {
	if (dx && edit_mode != MARK_PATTERN) {
		cursor_x = (cursor_x + dx + CHANNEL_COUNT) % CHANNEL_COUNT;
	}

	cursor_y0 = (cursor_y0 + dy0 + tune->table.size()) % tune->table.size();

	auto& line = tune->table[cursor_y0];
	int max_rows = std::max(1, get_max_rows(line, tune->patterns));
	cursor_y1 = (cursor_y1 + dy1 + max_rows) % max_rows;

	do_scroll();
}


void PatternWin::key(int ch) {
	auto& line = tune->table[cursor_y0];
	auto& pat_name = line[cursor_x];
	auto pat = tune->patterns.count(pat_name) ? &tune->patterns[pat_name] : nullptr;
	auto row = (pat && cursor_y1 < (int) pat->size()) ? &pat->at(cursor_y1) : nullptr;

	if (edit_mode == PATTERN) { // edit pattern name
		if ((isalnum(ch) || strchr("_-", ch)) && pat_name.size() < PATTERN_CHAR_WIDTH) {
			pat_name += ch;
		}
		else if (ch == KEY_BACKSPACE && pat_name.size() > 0) pat_name.pop_back();
		else if (ch == KEY_ESCAPE) {
			edit_mode = OFF;
			pat_name.assign(old_name);
		}
		else if (ch == '\n') {
			edit_mode = OFF;

			// new pattern
			if (old_name == "" && pat_name != "" && tune->patterns.count(pat_name) == 0) {
				tune->patterns[pat_name].resize(16);
			}

			// TODO: delete pattern, if unused
			if (old_name != "" && pat_name != "") {

			}
		}
		return;
	}
	else if (edit_mode == MACRO) { // edit macro name
		auto& macro_name = row->macros[0];
		if ((isalnum(ch) || strchr("_-", ch)) && macro_name.size() < MACRO_CHAR_WIDTH) {
			macro_name += ch;
		}
		else if (ch == KEY_BACKSPACE && macro_name.size() > 0) macro_name.pop_back();
		else if (ch == 27) {
			edit_mode = OFF;
			macro_name.assign(old_name);
		}
		else if (ch == '\n') {
			edit_mode = OFF;
			macro = macro_name;
		}
		return;
	}
	else if (edit_mode == MARK_PATTERN) {
		switch (ch) {
		case KEY_UP:
			move_cursor(0, 0, -1);
			return;

		case KEY_DOWN:
			move_cursor(0, 0, 1);
			return;

		case KEY_PPAGE:
			move_cursor(0, 0, -4);
			return;
		case KEY_NPAGE:
			move_cursor(0, 0, 4);
			return;

		case KEY_RIGHT:
			move_cursor(1, 0, 0);
			return;

		case KEY_LEFT:
			move_cursor(-1, 0, 0);
			return;


		case 'V':
			mark_y = 0;
			cursor_y1 = std::max<int>(0, pat->size() - 1);
			do_scroll();
			return;

		case 'y': {
				// copy pattern to buffer
				edit_mode = OFF;
				buffer_pat.clear();
				int i = std::min<int>(cursor_y1, mark_y);
				int limit = std::max<int>(cursor_y1, mark_y) + 1;
				while (i < limit && i < (int) pat->size()) {
					buffer_pat.push_back(pat->at(i++));
				}
			}
			return;

		case KEY_ESCAPE:
			edit_mode = OFF;
			return;


		case KEY_BACKSPACE: {
				edit_mode = OFF;
				int i = std::min<int>(cursor_y1, mark_y);
				int limit = std::max<int>(cursor_y1, mark_y);
				while (i < limit && i < (int) pat->size()) {
					auto& row = pat->at(i++);
					row.note = 0;
					for (auto& m : row.macros) m = "";
				}
			}
			return;

		default: return;
		}
	}

	// edit_mode == OFF
	switch (ch) {
	case KEY_UP:
		move_cursor(0, 0, -1);
		return;

	case KEY_DOWN:
		move_cursor(0, 0, 1);
		return;

	case KEY_PPAGE:
		move_cursor(0, 0, -4);
		return;
	case KEY_NPAGE:
		move_cursor(0, 0, 4);
		return;

	case KEY_CTRL_UP:
		move_cursor(0, -1, 0);
		return;

	case KEY_CTRL_DOWN:
		move_cursor(0, 1, 0);
		return;

	case KEY_CTRL_RIGHT:
	case KEY_RIGHT:
		move_cursor(1, 0, 0);
		return;

	case KEY_LEFT:
	case KEY_CTRL_LEFT:
		move_cursor(-1, 0, 0);
		return;

	case 'V':
		if (pat) {
			edit_mode = MARK_PATTERN;
			mark_y = cursor_y1;
		}
		return;

	case 'P':
		if (pat) {
			int len = cursor_y1 + buffer_pat.size();
			if ((int) pat->size() < len) pat->resize(len);
			int i = cursor_y1;
			for (auto& row : buffer_pat) {
				pat->at(i++) = row;
			}
		}
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
		tune->table.push_back({});
		return;
	case KEY_CTRL_Y:
		if (tune->table.size() > 1) {
			tune->table.pop_back();
			cursor_y0 = std::min(cursor_y0, (int) tune->table.size() - 1);
		}
		return;

	case KEY_CTRL_N:
		edit_mode = PATTERN;
		old_name = pat_name;
		return;

	case 'N':
		if (row) {
			edit_mode = MACRO;
			old_name = row->macros[0];
		}
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

	case '+': {
			auto it = tune->macros.find(macro);
			it++;
			if (macro == "") it = tune->macros.begin();
			if (it != tune->macros.end()) macro = it->first;
			else macro = "";
		}
		break;
	case '-': {
			MacroMap::reverse_iterator it(tune->macros.find(macro));
			if (macro == "") it = tune->macros.rbegin();
			if (it != tune->macros.rend()) macro = it->first;
			else macro = "";
		}
		break;


	case 'S':
		msg_win.say("Saving tune file... ");
		if (!save_tune(*tune, tunefile)) msg_win.append("error.");
		else msg_win.append("done.");

		break;

	case ' ':
		server.set_playing(!server.get_playing(), cursor_y0);
		break;

	case '\0':
		server.set_playing(!server.get_playing(), cursor_y0, true);

	default: break;
	}


	if (edit_mode != OFF || !row) return;


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
