#include <curses.h>
#include <ctype.h>
#include <string.h>

#include "server.h"
#include "messagewin.h"
#include "patternwin.h"
#include "styles.h"


bool operator==(const Row& a, const Row& b) {
	return a.note == b.note && a.macros == b.macros;
}
bool operator!=(const Row& a, const Row& b) { return !(a == b); }

void EditCommand::m_restore_cursor(PatternWin& win) const {
	win.m_cursor_y0 = m_cursor_y0;
	win.m_cursor_x = m_cursor_x;
	if (m_type != DELETE_LINE && m_type != INSERT_LINE) {
		win.m_cursor_y1 = m_cursor_y1;
	}
	win.m_scroll();
}

bool paste_region(Tune& tune, int line_nr, int x, int y,
				  const std::vector<Pattern>& src, std::vector<int>* length_diffs=nullptr)
{
	if (length_diffs) length_diffs->resize(src.size());
	bool same = true;
	auto& line = tune.table[line_nr];
	for (int c = 0; c < (int) src.size(); c++) {
		auto it = tune.patterns.find(line[(x + c) % CHANNEL_COUNT]);
		if (it != tune.patterns.end()) {
			auto& pat = it->second;
			auto& buffer = src[c];

			if (length_diffs) length_diffs->at(c) = 0;
			if (pat.size() < y + buffer.size()) {
				if (length_diffs) length_diffs->at(c) = y + buffer.size() - pat.size();
				pat.resize(y + buffer.size());
				same = false;
			}
			for (int i = 0; i < (int) buffer.size(); i++) {
				if (pat[y + i] != buffer[i]) {
					pat[y + i] = buffer[i];
					same = false;
				}
			}
		}
	}
	return same;
}


bool yank_region(Tune& tune, int line_nr, int x0, int y0, int x1, int y1,
				 std::vector<Pattern>& dst, bool clear=false)
{
	int same = true;
	auto& line = tune.table[line_nr];
	dst.resize(x1 - x0);
	for (int c = x0; c < x1; c++) {
		auto& buffer = dst[c - x0];
		buffer.clear();
		auto it = tune.patterns.find(line[c]);
		if (it != tune.patterns.end()) {
			auto& pat = it->second;
			for (int i = y0; i < y1 && i < (int) pat.size(); i++) {
				buffer.push_back(pat[i]);
				if (clear) {
					if (pat[i] != Row()) {
						pat[i] = Row();
						same = false;
					}
				}
			}
		}
	}
	return same;
}


bool EditCommand::exec(PatternWin& win, Execution e) {
	if (e == ECE_DO) {
		if (m_type == RECORD_ROW) {
			server.get_nearest_row(m_cursor_y0, m_cursor_y1);
			m_cursor_x = win.m_cursor_x;
		}
		else if (m_type == YANK_REGION || m_type == TRANSPOSE_REGION) {
			m_cursor_x	= win.m_mark_x_begin();
			m_cursor_y0	= win.m_cursor_y0;
			m_cursor_y1	= win.m_mark_y_begin();
		}
		else {
			m_cursor_x	= win.m_cursor_x;
			m_cursor_y0	= win.m_cursor_y0;
			m_cursor_y1	= win.m_cursor_y1;
		}

	}


	auto& table = win.m_tune->table;
	auto& patterns = win.m_tune->patterns;
	auto& line = table[m_cursor_y0];
	auto& pat_name = line[m_cursor_x];
	auto pat = patterns.count(pat_name) ? &patterns[pat_name] : nullptr;
	auto row = (pat && m_cursor_y1 < (int) pat->size()) ? &pat->at(m_cursor_y1) : nullptr;


	switch (m_type) {
	case SET_ROW:
	case RECORD_ROW:
		if (!row) return false;
		if (e == ECE_DO) m_prev_row = *row;
		else m_restore_cursor(win);
		if (e != ECE_UNDO) {
			if (*row == m_row) return false;
			*row = m_row;
		}
		else *row = m_prev_row;
		return true;


	case SET_MACRO:
		if (!row) return false;
		if (e == ECE_DO) {
			m_prev_row = *row;
			m_row = *row;
			m_row.macros[m_index] = win.m_macro;
		}
		else m_restore_cursor(win);
		if (e != ECE_UNDO) {
			if (*row == m_row) return false;
			*row = m_row;
		}
		else *row = m_prev_row;
		return true;


	case DELETE_ROW:
		if (e == ECE_DO) {
			if (!pat || pat->size() <= 1) return false;
//			m_cursor_y1 = win.m_cursor_y1 = m_index;
			if (m_cursor_y1 >= (int) pat->size() - 1) {
				m_cursor_y1 = win.m_cursor_y1 = pat->size() - 1;
			}
			m_prev_row = pat->at(m_cursor_y1);
		}
		else m_restore_cursor(win);
		if (e != ECE_UNDO) {
			pat->erase(pat->begin() + m_cursor_y1);
			if (m_cursor_y1 > (int) pat->size() - 1) win.m_move_cursor(0, 0, -1);
		}
		else pat->insert(pat->begin() + m_cursor_y1, m_prev_row);
		return true;


	case INSERT_ROW:
		if (!pat) return false;
		if (e == ECE_DO) {
			m_cursor_y1 = win.m_cursor_y1 = m_index;
			if (m_cursor_y1 >= (int) pat->size()) {
				m_cursor_y1 = win.m_cursor_y1 = pat->size();
			}
		}
		else m_restore_cursor(win);
		if (e != ECE_UNDO) {
			pat->insert(pat->begin() + m_cursor_y1, Row());
		}
		else {
			pat->erase(pat->begin() + m_cursor_y1);
			if (m_cursor_y1 > (int) pat->size() - 1) win.m_move_cursor(0, 0, -1);
		}
		return true;



	case YANK_REGION:
		if (e == ECE_DO) {
			int xe = win.m_mark_x_end();
			int ye = win.m_mark_y_end();
			if (yank_region(*win.m_tune, m_cursor_y0, m_cursor_x, m_cursor_y1, xe, ye,
				win.m_pattern_buffer, m_clear)) return false;
			m_prev_region = win.m_pattern_buffer;
			yank_region(*win.m_tune, m_cursor_y0, m_cursor_x, m_cursor_y1, xe, ye, m_region);
		}
		else m_restore_cursor(win);
		if (e == ECE_REDO) paste_region(*win.m_tune, m_cursor_y0, m_cursor_x, m_cursor_y1, m_region);
		else if (e == ECE_UNDO) paste_region(*win.m_tune, m_cursor_y0, m_cursor_x, m_cursor_y1, m_prev_region);
		return true;


	case PASTE_REGION:
		if (e == ECE_DO) {
			m_region = win.m_pattern_buffer;
			int len = 0;
			for (auto& b : m_region) len = std::max(len, (int) b.size());
			yank_region(*win.m_tune, m_cursor_y0, m_cursor_x, m_cursor_y1, m_cursor_x + m_region.size(),
				m_cursor_y1 + len, m_prev_region);
			if (paste_region(*win.m_tune, m_cursor_y0, m_cursor_x, m_cursor_y1, m_region, &m_length_diffs)) {
				return false;
			}
		}
		else m_restore_cursor(win);
		if (e == ECE_REDO) paste_region(*win.m_tune, m_cursor_y0, m_cursor_x, m_cursor_y1, m_region);
		else if (e == ECE_UNDO) {
			paste_region(*win.m_tune, m_cursor_y0, m_cursor_x, m_cursor_y1, m_prev_region);
			// trim patterns
			for (int c = 0; c < m_region.size(); c++) {
				auto it = patterns.find(line[m_cursor_x + c]);
				if (it != patterns.end()) {
					auto& pat = it->second;
					pat.resize(pat.size() - m_length_diffs[c]);
				}
			}
		}
		return true;

	case TRANSPOSE_REGION:
		if (e == ECE_DO) {
			int xe = win.m_mark_x_end();
			int ye = win.m_mark_y_end();
			yank_region(*win.m_tune, m_cursor_y0, m_cursor_x, m_cursor_y1, xe, ye, m_prev_region);
			for (int c = m_cursor_x; c < xe; c++) {
				auto it = patterns.find(line[c]);
				if (it != patterns.end()) {
					auto& pat = it->second;
					for (int i = m_cursor_y1; i < ye && i < (int) pat.size(); i++) {
						auto& row = pat[i];
						if (row.note <= 0) continue;
						row.note = clamp(row.note + m_index, 1, 120);
					}
				}
			}
			yank_region(*win.m_tune, m_cursor_y0, m_cursor_x, m_cursor_y1, xe, ye, m_region);
			if (m_region == m_prev_region) return false;
		}
		else m_restore_cursor(win);
		if (e == ECE_REDO) paste_region(*win.m_tune, m_cursor_y0, m_cursor_x, m_cursor_y1, m_region);
		else if (e == ECE_UNDO) paste_region(*win.m_tune, m_cursor_y0, m_cursor_x, m_cursor_y1, m_prev_region);
		return true;


	case DELETE_LINE:
		if (e == ECE_DO) {
			if (table.size() <= 1) return false;
			if (m_cursor_y0 >= (int) table.size() - 1) {
				m_cursor_y0 = win.m_cursor_y0 = table.size() - 1;
			}
			m_prev_line = line;
		}
		else m_restore_cursor(win);
		if (e != ECE_UNDO) {
			table.erase(table.begin() + m_cursor_y0);
			if (m_cursor_y0 > (int) table.size() - 1) win.m_move_cursor(0, -1, 0);
		}
		else table.insert(table.begin() + m_cursor_y0, m_prev_line);
		return true;


	case INSERT_LINE:
		if (e == ECE_DO) {
			m_cursor_y0 = win.m_cursor_y0 = m_index;
			if (m_cursor_y0 >= (int) table.size()) {
				m_cursor_y0 = win.m_cursor_y0 = table.size();
			}
		}
		else m_restore_cursor(win);
		if (e != ECE_UNDO) {
			TableLine l;
			table.insert(table.begin() + m_cursor_y0, l);
		}
		else {
			table.erase(table.begin() + m_cursor_y0);
			if (m_cursor_y0 > (int) table.size() - 1) win.m_move_cursor(0, -1, 0);
		}
		return true;


	default:
		msg_win.say("Invalid command");
		return false;
	}
}



void PatternWin::resize() {
	m_width = COLS - m_left;
	m_height = LINES - m_top - 2;
	m_scroll_x_view = (m_width - 4) / (CHAN_CHAR_WIDTH + 1);

	m_scroll_y0_view = 8;
	m_scroll_y1_view = m_height - m_scroll_y0_view - 5;

	m_scroll();
}

void PatternWin::m_scroll() {
	if (m_scroll_x > m_cursor_x) m_scroll_x = m_cursor_x;
	if (m_scroll_x < m_cursor_x - m_scroll_x_view + 1) {
		m_scroll_x = m_cursor_x - m_scroll_x_view + 1;
	}
	if (m_scroll_y0 > m_cursor_y0) m_scroll_y0 = m_cursor_y0;
	if (m_scroll_y0 < m_cursor_y0 - m_scroll_y0_view + 1) {
		m_scroll_y0 = m_cursor_y0 - m_scroll_y0_view + 1;
	}
	if (m_scroll_y1 > m_cursor_y1) m_scroll_y1 = m_cursor_y1;
	if (m_scroll_y1 < m_cursor_y1 - m_scroll_y1_view + 1) {
		m_scroll_y1 = m_cursor_y1 - m_scroll_y1_view + 1;
	}
}

void PatternWin::m_move_cursor(int dx, int dy0, int dy1) {
	if (dx) m_cursor_x = (m_cursor_x + dx + CHANNEL_COUNT) % CHANNEL_COUNT;
	if (dy0) m_cursor_y0 = (m_cursor_y0 + dy0 + m_tune->table.size()) % m_tune->table.size();
	if (dy1) {
		int max_rows = std::max(1, get_max_rows(*m_tune, m_cursor_y0));
		m_cursor_y1 = (m_cursor_y1 + dy1 + max_rows) % max_rows;
	}
	m_scroll();
}

void PatternWin::draw() {
	int server_row = server.get_row();
	int server_line = server.get_line();

	// follow
	if (m_edit_mode == EM_RECORD) {
		m_cursor_y0 = server_line;
		m_cursor_y1 = server_row;
		m_scroll();
	}

	set_style(S_FRAME);
	mvprintw(m_top, m_left, "   ");
	addch(ACS_ULCORNER);
	mvprintw(m_top + 1, m_left, "   ");
	addch(ACS_VLINE);
	move(m_top + 2, m_left);
	addch(ACS_ULCORNER);
	addchs(ACS_HLINE, 2);
	addch(ACS_PLUS);
	for (int r = 0; r < m_scroll_y0_view; r++) {
		move(m_top + r + 3, m_left);
		addch(ACS_VLINE);
		if (r + m_scroll_y0 < (int) m_tune->table.size()) {
			set_style(r == m_cursor_y0 ? S_HL_NORMAL : S_NORMAL);
			printw("%02X", r + m_scroll_y0);
			set_style(S_FRAME);
		}
		else printw("  ");
		set_style(S_FRAME);
		addch(ACS_VLINE);
	}



	auto& line = m_tune->table[m_cursor_y0];
	int max_rows = get_max_rows(*m_tune, m_cursor_y0);
	int y1 = m_top + m_scroll_y0_view + 3;

	move(y1, m_left);
	addch(ACS_LTEE);
	addchs(ACS_HLINE, 2);
	addch(ACS_PLUS);
	for (int r = 0; r < m_scroll_y1_view; r++) {
		int i = r + m_scroll_y1;
		move(y1 + r + 1, m_left);
		addch(ACS_VLINE);
		if (i < max_rows) {
			set_style(i == m_cursor_y1 ? S_HL_NORMAL : S_NORMAL);
			printw("%02X", i);
			set_style(S_FRAME);
		}
		else printw("  ");
		set_style(S_FRAME);
		addch(ACS_VLINE);
	}

	move(m_top + m_height - 1, m_left);
	addch(ACS_LLCORNER);
	addchs(ACS_HLINE, 2);
	addch(ACS_BTEE);


	int x = m_left + 4;
	int chan_limit = std::min(m_scroll_x + m_scroll_x_view, (int) CHANNEL_COUNT);
	for (int chan_nr = m_scroll_x; chan_nr < chan_limit; chan_nr++, x += CHAN_CHAR_WIDTH + 1) {

		auto& pat_name = line[chan_nr];
		auto pat = m_tune->patterns.count(pat_name) ? &m_tune->patterns[pat_name] : nullptr;


		// table head

		move(m_top, x);
		addchs(ACS_HLINE, CHAN_CHAR_WIDTH);
		addch(chan_nr < chan_limit - 1 ? ACS_TTEE : ACS_URCORNER);
		move(m_top + 1, x);


		int level = clamp(server.get_chan_level(chan_nr)) * (CHAN_CHAR_WIDTH - 1);
		for (int i = 0; i < CHAN_CHAR_WIDTH - 1; i++) {
			set_style(i < level ? S_LEVEL : S_NORMAL);
			int p = CHAN_CHAR_WIDTH / 2 - 3;
			addch(!server.get_muted(chan_nr) || i < p || i > p + 4 ? ' ' : "MUTED"[i - p]);
		}

		printw("%X", chan_nr);
		set_style(S_FRAME);
		addch(ACS_VLINE);
		move(m_top + 2, x);
		addchs(ACS_HLINE, CHAN_CHAR_WIDTH);
		addch(chan_nr < chan_limit - 1 ? ACS_PLUS : ACS_RTEE);


		move(y1, x);
		addchs(ACS_HLINE, CHAN_CHAR_WIDTH);
		addch(chan_nr < chan_limit - 1 ? ACS_PLUS : ACS_RTEE);

		// top

		for (int r = 0; r < m_scroll_y0_view; r++) {
			int i = r + m_scroll_y0;
			move(m_top + r + 3, x);
			if (i < (int) m_tune->table.size()) {

				int style = S_PATTERN;
				if (i == server_line) style = S_PL_PATTERN;
				if (i == m_cursor_y0) style = S_HL_PATTERN;
				if (i == m_cursor_y0 && m_cursor_x == chan_nr) {
					style = (m_edit_mode == EM_PATTERN_NAME) ? S_ET_PATTERN :
							(m_edit_mode == EM_RECORD) ? S_RC_PATTERN : S_CS_PATTERN;
				}
				set_style(style);

				auto pn = m_tune->table[i][chan_nr];
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
		for (int r = 0; r < m_scroll_y1_view; r++) {
			int i = r + m_scroll_y1;
			move(y1 + r + 1, x);

			bool on_pat = (pat && i < (int) pat->size());

			int style = on_pat ? S_NOTE : S_FRAME;
			if (on_pat && i == server_row && m_tune->table[server_line][chan_nr] == pat_name) style = S_PL_NOTE;
			if (i == m_cursor_y1) style = S_HL_NOTE;
			if (i == m_cursor_y1 && m_cursor_x == chan_nr) {
				style = (m_edit_mode == EM_MACRO_NAME) ? S_ET_NOTE :
						(m_edit_mode == EM_RECORD) ? S_RC_NOTE :
						S_CS_NOTE;
			}
			if (m_edit_mode == EM_MARK_PATTERN
			&& m_mark_x_begin() <= chan_nr && chan_nr < m_mark_x_end()
			&& m_mark_y_begin() <= i && i < m_mark_y_end()) style = S_MK_NOTE;
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
					std::string m_macro = row.macros[m];
					printw(" %s", m_macro.c_str());
					addchs('.', MACRO_CHAR_WIDTH - m_macro.size());
				}
			}
			else {
				addchs(' ', CHAN_CHAR_WIDTH);
			}
			set_style(S_FRAME);
			addch(ACS_VLINE);
		}

		set_style(S_FRAME);
		move(m_top + m_height - 1, x);
		addchs(ACS_HLINE, CHAN_CHAR_WIDTH);
		addch(chan_nr < chan_limit - 1 ? ACS_BTEE : ACS_LRCORNER);

	}
	set_style(S_DEFAULT);
	for (int r = 0; r < m_height; r++) {
		move(m_top + r, x);
		addchs(' ', std::max(0, m_width - x));
	}



	// extra editing info
	set_style(S_FRAME);
	move(m_top + m_height - 1, m_left);
	addch(ACS_LTEE);
	move(m_top + m_height - 1, m_left + 3 + MACRO_CHAR_WIDTH);
	addch(ACS_TTEE);


	move(m_top + m_height, m_left);
	addch(ACS_VLINE);
	set_style(S_NOTE);
	addch('0' + m_octave);
	addch(' ');
	set_style(S_MACRO);
	printw("%s", m_macro.c_str());
	addchs('.', MACRO_CHAR_WIDTH - m_macro.size());
	set_style(S_FRAME);
	addch(ACS_VLINE);


	move(m_top + m_height + 1, m_left);
	addch(ACS_LLCORNER);
	addchs(ACS_HLINE, 2 + MACRO_CHAR_WIDTH);
	addch(ACS_LRCORNER);

	set_style(S_NORMAL);
//	mvprintw(m_top + m_height + 1, m_left, "|%1d|%-*s|", m_octave, MACRO_CHAR_WIDTH, m_macro.c_str());


	// set cursor position

	if (m_edit_mode == EM_PATTERN_NAME) {
		curs_set(1);
		move(m_top + 3 + m_cursor_y0 - m_scroll_y0,
			m_left + (m_cursor_x - m_scroll_x) * (CHAN_CHAR_WIDTH + 1) + 4 + line[m_cursor_x].size());
	}
	else if (m_edit_mode == EM_MACRO_NAME) {
		curs_set(1);
		auto& pat_name = line[m_cursor_x];
		auto& pat = m_tune->patterns[pat_name];
		auto& row = pat[m_cursor_y1];
		move(m_top + 3 + m_scroll_y0_view + m_cursor_y1 - m_scroll_y1 + 1,
			m_left + (m_cursor_x - m_scroll_x) * (CHAN_CHAR_WIDTH + 1) + 4 + row.macros[0].size() + 4);
	}
	else curs_set(0);
}


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
	KEY_CTRL_D = 4,
	KEY_ESCAPE = 27,
	KEY_TAB = 9,
};

void PatternWin::key(int ch) {
	switch (m_edit_mode) {
	case EM_PATTERN_NAME:	m_key_pattern_name(ch); break;
	case EM_MACRO_NAME:		m_key_macro_name(ch); break;
	case EM_MARK_PATTERN:	m_key_mark_pattern(ch); break;
	case EM_RECORD:			m_key_record(ch); break;
	case EM_NORMAL:			m_key_normal(ch); break;
	}
}

typedef EditCommand EC;

void PatternWin::m_key_pattern_name(int ch) {
	auto& line = m_tune->table[m_cursor_y0];
	auto& pat_name = line[m_cursor_x];
	if ((isalnum(ch) || strchr("m_-+'\"*~!\\/#?$%&=<>", ch)) && pat_name.size() < PATTERN_CHAR_WIDTH) {
		pat_name += ch;
	}
	else if (ch == KEY_BACKSPACE && pat_name.size() > 0) pat_name.pop_back();
	else if (ch == KEY_ESCAPE) {
		m_edit_mode = EM_NORMAL;
		pat_name.assign(m_old_name);
	}
	else if (ch == '\n') {
		m_edit_mode = EM_NORMAL;

		if (pat_name != "" && m_tune->patterns.count(pat_name) == 0) {
			if (m_rename_pattern && m_old_name != "") { // rename pattern
				m_tune->patterns[pat_name] = m_tune->patterns[m_old_name];
			}
			else {
				// new pattern
				int len = std::max<int>(1, get_max_rows(*m_tune, m_cursor_y0));
				m_tune->patterns[pat_name].resize(len);
			}
		}
	}
}

void PatternWin::m_key_macro_name(int ch) {
	auto& line = m_tune->table[m_cursor_y0];
	auto& pat_name = line[m_cursor_x];
	auto& pat = m_tune->patterns[pat_name];
	auto& row = pat[m_cursor_y1];
	auto& macro_name = row.macros[0];

	if ((isalnum(ch) || strchr("m_-+'\"*~!\\/#?$%&=<>", ch)) && macro_name.size() < MACRO_CHAR_WIDTH) {
		macro_name += ch;
	}
	else if (ch == KEY_BACKSPACE && macro_name.size() > 0) macro_name.pop_back();
	else if (ch == KEY_ESCAPE) {
		m_edit_mode = EM_NORMAL;
		macro_name.assign(m_old_name);
	}
	else if (ch == '\n') {
		m_edit_mode = EM_NORMAL;
		m_macro = macro_name;
		macro_name.assign(m_old_name);
		m_edit<EC::SET_MACRO>(0);
	}
}

void PatternWin::m_key_mark_pattern(int ch) {
	switch (ch) {
	case KEY_UP:	m_move_cursor( 0, 0, -1); return;
	case KEY_DOWN:	m_move_cursor( 0, 0,  1); return;
	case KEY_PPAGE:	m_move_cursor( 0, 0, -4); return;
	case KEY_NPAGE:	m_move_cursor( 0, 0,  4); return;
	case KEY_RIGHT:	m_move_cursor( 1, 0,  0); return;
	case KEY_LEFT:	m_move_cursor(-1, 0,  0); return;
	case KEY_HOME:
		m_cursor_y1 = 0;
		m_scroll();
		return;
	case KEY_END:
		m_cursor_y1 = std::max(0, get_max_rows(*m_tune, m_cursor_y0) - 1);
		m_scroll();
		return;

	// mark whole pattern
	case 'V':
		m_mark_y = 0;
		m_cursor_y1 = std::max<int>(0, get_max_rows(*m_tune, m_cursor_y0) - 1);
		m_scroll();
		return;

	case KEY_ESCAPE:
		m_edit_mode = EM_NORMAL;
		return;

	// copy pattern to buffer
	case 'y':
	case 'd':
	case KEY_BACKSPACE:
		m_edit_mode = EM_NORMAL;
		m_edit<EC::YANK_REGION>(ch != 'y');
		return;

	// transpose
	case '>':
	case '<':
		m_edit<EC::TRANSPOSE_REGION>((ch == '>') - (ch == '<'));
		return;

	default: break;
	}
}

void PatternWin::m_key_rec_norm_common(int ch) {
	switch (ch) {
	case KEY_CTRL_RIGHT:
	case KEY_RIGHT:		m_move_cursor(1, 0, 0); return;
	case KEY_LEFT:
	case KEY_CTRL_LEFT:	m_move_cursor(-1, 0, 0); return;

	case '<':
		if(--m_octave < 0) m_octave = 0;
		return;
	case '>':
		if (++m_octave > 8) m_octave = 8;
		return;

	case '+': {
			auto it = m_tune->macros.find(m_macro);
			it++;
			if (m_macro == "") it = m_tune->macros.begin();
			if (it != m_tune->macros.end()) m_macro = it->first;
			else m_macro = "";
		}
		return;
	case '-': {
			MacroMap::reverse_iterator it(m_tune->macros.find(m_macro));
			if (m_macro == "") it = m_tune->macros.rbegin();
			if (it != m_tune->macros.rend()) m_macro = it->first;
			else m_macro = "";
		}
		return;

	case '\0':	// continue playing
		if (server.is_playing()) {
			m_edit_mode = EM_NORMAL;
			server.pause();
		}
		else server.play(server.get_line(), server.get_row());
		return;

	case ' ':	// play from the begining current line
		if (server.is_playing()) {
			m_edit_mode = EM_NORMAL;
			server.pause();
		}
		else server.play(m_cursor_y0);
		return;

	case '\n':	// loop current line
		if (server.is_playing()) {
			m_edit_mode = EM_NORMAL;
			server.pause();
		}
		else server.play(m_cursor_y0, 0, true);
		return;


	case 'M':	// mute
		server.set_muted(m_cursor_x, !server.get_muted(m_cursor_x));
		if (server.get_muted(m_cursor_x)) server.play_row(m_cursor_x, { -1 });
		return;
	case 'L':	// solo
		{
			int s = 0;
			for (int i = 0; i < CHANNEL_COUNT; i++) s += server.get_muted(i);
			if (!server.get_muted(m_cursor_x) && s == CHANNEL_COUNT - 1) {
				for (int i = 0; i < CHANNEL_COUNT; i++) server.set_muted(i, false);
			}
			else {
				for (int i = 0; i < CHANNEL_COUNT; i++) {
					server.set_muted(i, i != m_cursor_x);
					if (i != m_cursor_x) server.play_row(i, { -1 });
				}
			}
		}
		return;

	default: break;
	}
}

void PatternWin::m_key_record(int ch) {

	m_key_rec_norm_common(ch);
	if (ch == KEY_TAB) {
		m_edit_mode = EM_NORMAL;
		return;
	}

	if (ch < 32 || ch > 127) return;
	if (ch == '^') {
		Row row { -1 };
		m_edit<EC::RECORD_ROW>(row);
		server.play_row(m_cursor_x, row);
		return;
	}
	static const char* t1 = "ysxdcvgbhnjm,";
	static const char* t2 = "q2w3er5t6z7ui";
	const char* a = nullptr;
	int n;
	if ((a = strchr(t1, ch))) n = a - t1;
	else if ((a = strchr(t2, ch))) n = a - t2 + 12;
	if (a) {
		Row row { n + 1 + m_octave * 12 };
		row.macros[0] = m_macro;
		m_edit<EC::RECORD_ROW>(row);
		server.play_row(m_cursor_x, row);
	}
}

void PatternWin::m_key_normal(int ch) {
	auto& line = m_tune->table[m_cursor_y0];
	auto& pat_name = line[m_cursor_x];
	auto pat = m_tune->patterns.count(pat_name) ? &m_tune->patterns[pat_name] : nullptr;
	auto row = (pat && m_cursor_y1 < (int) pat->size()) ? &pat->at(m_cursor_y1) : nullptr;

	m_key_rec_norm_common(ch);

	switch (ch) {
	case KEY_UP:		m_move_cursor(0,  0, -1); return;
	case KEY_DOWN:		m_move_cursor(0,  0,  1); return;
	case KEY_PPAGE:		m_move_cursor(0,  0, -4); return;
	case KEY_NPAGE:		m_move_cursor(0,  0,  4); return;
	case KEY_CTRL_UP:	m_move_cursor(0, -1,  0); return;
	case KEY_CTRL_DOWN:	m_move_cursor(0,  1,  0); return;
	case KEY_HOME:
		m_cursor_y1 = 0;
		m_scroll();
		return;
	case KEY_END:
		m_cursor_y1 = std::max(0, get_max_rows(*m_tune, m_cursor_y0) - 1);
		m_scroll();
		return;

	case KEY_BACKSPACE:
		m_edit<EC::SET_ROW>(Row());
		return;

	case '!':			// set 1st macro
		m_edit<EC::SET_MACRO>(0);
		return;
	case '"':			// set 2nd macro
		m_edit<EC::SET_MACRO>(1);
		return;
	case 'I':			// edit m_macro name
		if (row) {
			m_edit_mode = EM_MACRO_NAME;
			m_old_name = row->macros[0];
		}
		return;

	case 'V':			// mark pattern
		m_edit_mode = EM_MARK_PATTERN;
		m_mark_x = m_cursor_x;
		m_mark_y = m_cursor_y1;
		return;

	case 'P':			// paste pattern
		m_edit<EC::PASTE_REGION>();
		return;

	case 'X':			// delete row
		m_edit<EC::DELETE_ROW>();
		return;

	case 'O':			// insert new row
		m_edit<EC::INSERT_ROW>(m_cursor_y1);
		return;

	case 'A':			// append new row
		m_edit<EC::INSERT_ROW>(m_cursor_y1 + 1);
		m_scroll();
		return;

	case KEY_CTRL_X:	// delete line
		m_edit<EC::DELETE_LINE>();
		return;

	case KEY_CTRL_O:	// insert new line
		m_edit<EC::INSERT_LINE>(m_cursor_y0);
		return;

	case KEY_CTRL_A:	// append new line
		m_edit<EC::INSERT_LINE>(m_cursor_y0 + 1);
		m_scroll();
		return;

	case KEY_CTRL_R:	// rename pattern
		m_edit_mode = EM_PATTERN_NAME;
		m_rename_pattern = true;
		m_old_name = pat_name;
		return;

	case KEY_CTRL_N:	// new pattern
		m_edit_mode = EM_PATTERN_NAME;
		m_rename_pattern = false;
		m_old_name = pat_name;
		return;

	case KEY_CTRL_D: 	// duplicate above pattern with auto naming
		if (pat) return;
		for (int i = m_cursor_y0; i >= 0; i--) {
			auto pn = m_tune->table[i][m_cursor_x];
			if (pn != "") {
				auto& p = m_tune->patterns[pn];
				auto pos = pn.find_last_not_of("0123456789");
				auto suffix = pn.substr(pos + 1);
				if (suffix == "") suffix = "0";
				else pn.erase(pos + 1);
				while (m_tune->patterns.find(pn + suffix) != m_tune->patterns.end()) {
					auto n = std::to_string(std::stoi(suffix) + 1);
					suffix = std::string(std::max<int>(0, suffix.size() - n.size()), '0') + n;
				}
				pn += suffix;
				m_tune->table[m_cursor_y0][m_cursor_x] = pn;
				m_tune->patterns[pn] = p;
				return;
			}
		}
		return;

	case KEY_TAB:
		m_edit_mode = EM_RECORD;
		if (!server.is_playing()) server.play(m_cursor_y0, m_cursor_y1);
		return;

	case 'S':
		strip_tune(*m_tune);
		msg_win.say("Saving tune file... ");
		if (!save_tune(*m_tune, m_tunefile)) msg_win.append("error.");
		else msg_win.append("done.");
		return;

	case 'U':
		m_undo();
		return;
	case 'R':
		m_redo();
		return;


	default: break;
	}


	if (ch < 32 || ch > 127) return;
	if (ch == '^') {
		Row row { -1 };
		m_edit<EC::SET_ROW>(row);
		server.play_row(m_cursor_x, row);
		return;
	}
	static const char* t1 = "ysxdcvgbhnjm,";
	static const char* t2 = "q2w3er5t6z7ui";
	const char* a = nullptr;
	int n;
	if ((a = strchr(t1, ch))) n = a - t1;
	else if ((a = strchr(t2, ch))) n = a - t2 + 12;
	if (a) {
		Row row { n + 1 + m_octave * 12 };
		row.macros[0] = m_macro;
		m_edit<EC::SET_ROW>(row);
		server.play_row(m_cursor_x, row);
	}
}

void PatternWin::midi(int type, int value) {

	Row row;
	int chan;

	if (type == 128) { // note off event
		if (m_note_to_chan[value] == -1) return;
		chan = m_note_to_chan[value];
		m_chan_to_note[chan] = -1;
		m_note_to_chan[value] = -1;
		row.note = -1;
	}
	else if (type == 144) {

		chan = m_cursor_x;
		for (int i = 1; i < POLYPHONY; i++) {
			if (m_chan_to_note[chan] == -1) break;
			chan = (chan + 1) % CHANNEL_COUNT;
		}
		int old_note = m_chan_to_note[chan];
		if (old_note != -1) m_note_to_chan[old_note] = -1;
		m_chan_to_note[chan] = value;
		m_note_to_chan[value] = chan;

		row.note = value + 1;
		row.macros[0] = m_macro;
	}
	else return;

	server.play_row(chan, row);

	if (row.note > 0 && m_edit_mode == EM_NORMAL) {
		m_edit<EC::SET_ROW>(row);
	}
	else if (m_edit_mode == EM_RECORD) {

		// record note off event only if no other voice active
		if (row.note == -1) {
			for (int n : m_chan_to_note) if (n != -1) return;
		}

		m_edit<EC::RECORD_ROW>(row);
	}
}
