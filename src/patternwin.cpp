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
	KEY_CTRL_A = 1,
	KEY_CTRL_O = 15,
	KEY_CTRL_R = 18,
	KEY_CTRL_X = 24,
	KEY_CTRL_N = 14,
	KEY_ESCAPE = 27,
	KEY_TAB = 9,
};


void PatternWin::resize() {
	width = COLS - left;
	height = LINES - top - 2;
	scroll_x_view = (width - 4) / (CHAN_CHAR_WIDTH + 1);

	scroll_y0_view = 8;
	scroll_y1_view = height - scroll_y0_view - 5;

	do_scroll();
}


void PatternWin::draw() {
	int server_row = server.get_row();
	int server_block = server.get_block();

	// follow
	if (edit_mode == EM_RECORD) {
		cursor_y0 = server_block;
		cursor_y1 = server_row;
		do_scroll();
	}

	set_style(S_FRAME);
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
			set_style(r == cursor_y0 ? S_HL_NORMAL : S_NORMAL);
			printw("%02X", r + scroll_y0);
			set_style(S_FRAME);
		}
		else printw("  ");
		set_style(S_FRAME);
		addch(ACS_VLINE);
	}



	auto& line = tune->table[cursor_y0];
	int max_rows = get_max_rows(*tune, cursor_y0);
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
			set_style(i == cursor_y1 ? S_HL_NORMAL : S_NORMAL);
			printw("%02X", i);
			set_style(S_FRAME);
		}
		else printw("  ");
		set_style(S_FRAME);
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
		set_style(S_LEVEL);
		addchs(' ', level);
		set_style(S_NORMAL);
		addchs(' ', CHAN_CHAR_WIDTH - 1 - level);
		printw("%X", chan_nr);
		set_style(S_FRAME);
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

				int style = S_MACRO;
				if (i == server_block) style = S_PL_MACRO;
				if (i == cursor_y0) style = S_HL_MACRO;
				if (i == cursor_y0 && cursor_x == chan_nr) {
					style = (edit_mode == EM_PATTERN_NAME) ? S_ET_MACRO : S_CS_MACRO;
				}
				set_style(style);

				auto pn = tune->table[i][chan_nr];
				printw("%s", pn.c_str());
				addchs(pn == "" ? ' ' : '.', CHAN_CHAR_WIDTH - pn.size());
			}
			else {
				set_style(S_FRAME);
				addchs(' ', CHAN_CHAR_WIDTH);
			}
			set_style(S_FRAME);
			addch(ACS_VLINE);
		}

		// bottom
		for (int r = 0; r < scroll_y1_view; r++) {
			int i = r + scroll_y1;
			move(y1 + r + 1, x);

			bool on_pat = (pat && i < (int) pat->size());

			int style = on_pat ? S_NOTE : S_FRAME;
			if (on_pat && i == server_row && tune->table[server_block][chan_nr] == pat_name) style = S_PL_NOTE;
			if (i == cursor_y1) style = S_HL_NOTE;
			if (i == cursor_y1 && cursor_x == chan_nr) {
				style = (edit_mode == EM_MACRO_NAME) ? S_ET_NOTE :
						(edit_mode == EM_RECORD) ? S_RC_NOTE :
						S_CS_NOTE;
			}
			if (edit_mode == EM_MARK_PATTERN
			&& mark_x_begin() <= chan_nr && chan_nr < mark_x_end()
			&& mark_y_begin() <= i && i < mark_y_end()) style = S_MK_NOTE;
			set_style(style);

			if (on_pat) {
				auto& row = pat->at(i);

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
				addchs(' ', CHAN_CHAR_WIDTH);
			}
			set_style(S_FRAME);
			addch(ACS_VLINE);
		}

		set_style(S_FRAME);
		move(top + height - 1, x);
		addchs(ACS_HLINE, CHAN_CHAR_WIDTH);
		addch(chan_nr < chan_limit - 1 ? ACS_BTEE : ACS_LRCORNER);

	}
	set_style(S_DEFAULT);
	for (int r = 0; r < height; r++) {
		move(top + r, x);
		addchs(' ', std::max(0, width - x));
	}


	mvprintw(top - 1, left + 20, "Oct:%d|Macro:%-8s", octave, macro.c_str());


	if (edit_mode == EM_PATTERN_NAME) {
		curs_set(1);
		move(4 + cursor_y0 - scroll_y0,
			(cursor_x - scroll_x) * (CHAN_CHAR_WIDTH + 1) + 4 + line[cursor_x].size());
	}
	else if (edit_mode == EM_MACRO_NAME) {
		curs_set(1);
		auto& pat_name = line[cursor_x];
		auto& pat = tune->patterns[pat_name];
		auto& row = pat[cursor_y1];
		move(4 + scroll_y0_view + cursor_y1 - scroll_y1 + 1,
			(cursor_x - scroll_x) * (CHAN_CHAR_WIDTH + 1) + 4 + row.macros[0].size() + 4);
	}
	else curs_set(0);
}

void PatternWin::key_pattern_name(int ch) {
	auto& line = tune->table[cursor_y0];
	auto& pat_name = line[cursor_x];
	if ((isalnum(ch) || strchr("_-", ch)) && pat_name.size() < PATTERN_CHAR_WIDTH) {
		pat_name += ch;
	}
	else if (ch == KEY_BACKSPACE && pat_name.size() > 0) pat_name.pop_back();
	else if (ch == KEY_ESCAPE) {
		edit_mode = EM_NORMAL;
		pat_name.assign(old_name);
	}
	else if (ch == '\n') {
		edit_mode = EM_NORMAL;

		if (pat_name != "" && tune->patterns.count(pat_name) == 0) {
			if (rename_pattern && old_name != "") { // rename pattern
				tune->patterns[pat_name] = tune->patterns[old_name];
			}
			else {
				// new pattern
				int len = std::max<int>(1, get_max_rows(*tune, cursor_y0));
				tune->patterns[pat_name].resize(len);
			}
		}
	}
}

void PatternWin::key_macro_name(int ch) {
	auto& line = tune->table[cursor_y0];
	auto& pat_name = line[cursor_x];
	auto& pat = tune->patterns[pat_name];
	auto& row = pat[cursor_y1];
	auto& macro_name = row.macros[0];

	if ((isalnum(ch) || strchr("_-", ch)) && macro_name.size() < MACRO_CHAR_WIDTH) {
		macro_name += ch;
	}
	else if (ch == KEY_BACKSPACE && macro_name.size() > 0) macro_name.pop_back();
	else if (ch == 27) {
		edit_mode = EM_NORMAL;
		macro_name.assign(old_name);
	}
	else if (ch == '\n') {
		edit_mode = EM_NORMAL;
		macro = macro_name;
	}
}

void PatternWin::key_mark_pattern(int ch) {
	auto& line = tune->table[cursor_y0];
	switch (ch) {

	case KEY_UP:	move_cursor(0, 0, -1); break;
	case KEY_DOWN:	move_cursor(0, 0, 1); break;
	case KEY_PPAGE:	move_cursor(0, 0, -4); break;
	case KEY_NPAGE:	move_cursor(0, 0, 4); break;
	case KEY_RIGHT:	move_cursor(1, 0, 0); break;
	case KEY_LEFT:	move_cursor(-1, 0, 0); break;

	// mark whole pattern
	case 'V':
		mark_y = 0;
		cursor_y1 = std::max<int>(0, get_max_rows(*tune, cursor_y0) - 1);
		do_scroll();
		break;

	case KEY_ESCAPE:
		edit_mode = EM_NORMAL;
		break;

	// copy pattern to buffer
	case 'y':
	case KEY_BACKSPACE:
		edit_mode = EM_NORMAL;
		pattern_buffer.resize(mark_x_end() - mark_x_begin());
		for (int c = mark_x_begin(); c < mark_x_end(); c++) {
			auto& buffer = pattern_buffer[c - mark_x_begin()];
			buffer.clear();
			auto it = tune->patterns.find(line[c]);
			if (it != tune->patterns.end()) {
				auto& pat = it->second;
				for (int i = mark_y_begin(); i < mark_y_end() && i < (int) pat.size(); i++) {
					buffer.push_back(pat[i]);
					if (ch == KEY_BACKSPACE) pat[i] = Row();
				}
			}
		}
		break;

	// transpose
	case '>':
	case '<':
		for (int c = mark_x_begin(); c < mark_x_end(); c++) {
			auto it = tune->patterns.find(line[c]);
			if (it != tune->patterns.end()) {
				auto& pat = it->second;
				for (int i = mark_y_begin(); i < mark_y_end() && i < (int) pat.size(); i++) {
					auto& row = pat[i];
					if (row.note <= 0) continue;
					if (ch == '>' && row.note < 120) row.note++;
					if (ch == '<' && row.note > 1) row.note--;
				}
			}
		}
		break;

	default: break;
	}
}

void PatternWin::key_rec_norm_common(int ch) {
	switch (ch) {
	case KEY_CTRL_RIGHT:
	case KEY_RIGHT:		move_cursor(1, 0, 0); return;
	case KEY_LEFT:
	case KEY_CTRL_LEFT:	move_cursor(-1, 0, 0); return;

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

	case ' ':
		if (server.is_playing()) server.stop();
		else server.play(cursor_y0);
		break;

	case '\0':
		if (server.is_playing()) server.stop();
		else server.play(cursor_y0, true);
		break;

	default: break;
	}
}

void PatternWin::key_record(int ch) {

	key_rec_norm_common(ch);
	if (ch == KEY_TAB) {
		edit_mode = EM_NORMAL;
		return;
	}

	auto row = server.get_nearest_row(cursor_x);
	Row r;
	if (!row) row = &r;

	// TODO
	if (ch < 32 || ch > 127) return;
	if (ch == '^') {
		row->note = -1;
		server.play_row(cursor_x, *row);
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
		server.play_row(cursor_x, *row);
	}
}

void PatternWin::midi_callback(int note) {
	Row row = { note };
	if (note > 0) row.macros[0] = macro;
	server.play_row(cursor_x, row);

	if (note > 0 && edit_mode == EM_NORMAL) {
		auto& pat_name = tune->table[cursor_y0][cursor_x];
		auto it = tune->patterns.find(pat_name);
		if (it != tune->patterns.end()) {
			auto& pat = it->second;
			if (cursor_y1 < (int) pat.size()) {
				pat[cursor_y1] = row;
			}
		}
	}
	else if (edit_mode == EM_RECORD) {
		auto r = server.get_nearest_row(cursor_x);
		if (r) *r = row;
	}
}

void PatternWin::key_normal(int ch) {
	auto& line = tune->table[cursor_y0];
	auto& pat_name = line[cursor_x];
	auto pat = tune->patterns.count(pat_name) ? &tune->patterns[pat_name] : nullptr;
	auto row = (pat && cursor_y1 < (int) pat->size()) ? &pat->at(cursor_y1) : nullptr;

	key_rec_norm_common(ch);

	switch (ch) {
	case KEY_UP:		move_cursor(0, 0, -1); return;
	case KEY_DOWN:		move_cursor(0, 0, 1); return;
	case KEY_PPAGE:		move_cursor(0, 0, -4); return;
	case KEY_NPAGE:		move_cursor(0, 0, 4); return;
	case KEY_CTRL_UP:	move_cursor(0, -1, 0); return;
	case KEY_CTRL_DOWN:	move_cursor(0, 1, 0); return;

	case KEY_BACKSPACE:
		if (row) *row = Row();
		return;

	case 'V':
		edit_mode = EM_MARK_PATTERN;
		mark_x = cursor_x;
		mark_y = cursor_y1;
		return;

	case 'P':
		for (int b = 0; b < (int) pattern_buffer.size(); b++) {
			auto it = tune->patterns.find(line[(cursor_x + b) % CHANNEL_COUNT]);
			if (it != tune->patterns.end()) {
				auto& pat = it->second;
				auto& buffer = pattern_buffer[b];
				if (pat.size() < cursor_y1 + buffer.size()) pat.resize(cursor_y1 + buffer.size());
				for (int i = 0; i < (int) buffer.size(); i++) {
					pat[cursor_y1 + i] = buffer[i];
				}
			}
		}
		return;

	case 'X':	// delete row
		if (pat) {
			if ((int) pat->size() > std::max<int>(1, cursor_y1)) {
				pat->erase(pat->begin() + cursor_y1);
				if (cursor_y1 > (int) pat->size() - 1) move_cursor(0, 0, -1);
			}
		}
		return;
	case 'O':	// insert new row
		if (pat) {
			if ((int) pat->size() < cursor_y1 + 1) pat->resize(cursor_y1 + 1);
			else pat->insert(pat->begin() + cursor_y1, Row());
		}
		return;
	case 'A':	// append new row
		if (pat) {
			if ((int) pat->size() < cursor_y1 + 1) pat->resize(cursor_y1 + 1);
			else pat->insert(pat->begin() + cursor_y1 + 1, Row());
		}
		return;

	case KEY_CTRL_X:	// delete block
		if ((int) tune->table.size() > std::max<int>(1, cursor_y0)) {
			tune->table.erase(tune->table.begin() + cursor_y0);
			if (cursor_y0 > (int) tune->table.size() - 1) move_cursor(0, -1, 0);
		}
		return;
	case KEY_CTRL_O:	// insert new block
		if ((int) tune->table.size() < cursor_y0 + 1) tune->table.resize(cursor_y0 + 1);
		else tune->table.insert(tune->table.begin() + cursor_y0, TableLine());
		return;
	case KEY_CTRL_A:	// append new block
		if ((int) tune->table.size() < cursor_y0 + 1) tune->table.resize(cursor_y0 + 1);
		else tune->table.insert(tune->table.begin() + cursor_y0 + 1, TableLine());
		return;

	case KEY_CTRL_R:
		edit_mode = EM_PATTERN_NAME;
		rename_pattern = true;
		old_name = pat_name;
		return;
	case KEY_CTRL_N:
		edit_mode = EM_PATTERN_NAME;
		rename_pattern = false;
		old_name = pat_name;
		return;
	case 'N':
		if (row) {
			edit_mode = EM_MACRO_NAME;
			old_name = row->macros[0];
		}
		return;

	case 'G': // new pattern with auto naming
		if (pat) return;
		for (int i = cursor_y0; i >= 0; i--) {
			auto pn = tune->table[i][cursor_x];
			if (pn != "") {
				auto pos = pn.find_last_not_of("0123456789");
				auto suffix = pn.substr(pos + 1);
				if (suffix == "") suffix = "0";
				else pn.erase(pos + 1);
				while (tune->patterns.find(pn + suffix) != tune->patterns.end()) {
					auto n = std::to_string(std::stoi(suffix) + 1);
					suffix = std::string(std::max<int>(0, suffix.size() - n.size()), '0') + n;
				}
				int len = std::max<int>(1, get_max_rows(*tune, cursor_y0));
				pn += suffix;
				tune->table[cursor_y0][cursor_x] = pn;
				tune->patterns[pn].resize(len);
				return;
			}
		}
		return;


	case KEY_TAB:
		edit_mode = EM_RECORD;
		return;

	case 'S':
		strip_tune(*tune);
		msg_win.say("Saving tune file... ");
		if (!save_tune(*tune, tunefile)) msg_win.append("error.");
		else msg_win.append("done.");
		return;

	default: break;
	}

	Row r;
	if (!row) row = &r;

	if (ch < 32 || ch > 127) return;
	if (ch == '^') {
		row->note = -1;
		server.play_row(cursor_x, *row);
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
		server.play_row(cursor_x, *row);
	}
}

void PatternWin::key(int ch) {
	switch (edit_mode) {
	case EM_PATTERN_NAME:	key_pattern_name(ch); break;
	case EM_MACRO_NAME:		key_macro_name(ch); break;
	case EM_MARK_PATTERN:	key_mark_pattern(ch); break;
	case EM_RECORD:			key_record(ch); break;
	case EM_NORMAL:			key_normal(ch); break;
	}
}

void PatternWin::move_cursor(int dx, int dy0, int dy1) {
	if (dx) cursor_x = (cursor_x + dx + CHANNEL_COUNT) % CHANNEL_COUNT;
	if (dy0) cursor_y0 = (cursor_y0 + dy0 + tune->table.size()) % tune->table.size();
	if (dy1) {
		int max_rows = std::max(1, get_max_rows(*tune, cursor_y0));
		cursor_y1 = (cursor_y1 + dy1 + max_rows) % max_rows;
	}
	do_scroll();
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
