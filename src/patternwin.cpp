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

void EditCommand::_restore_cursor(PatternWin& win) const {
	win._cursor_y0 = _cursor_y0;
	win._cursor_x = _cursor_x;
	if (_type != DELETE_LINE && _type != INSERT_LINE) {
		win._cursor_y1 = _cursor_y1;
	}
	win._scroll();
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
		if (_type == RECORD_ROW) {
			server.get_nearest_row(_cursor_y0, _cursor_y1);
			_cursor_x = win._cursor_x;
		}
		else if (_type == YANK_REGION || _type == TRANSPOSE_REGION) {
			_cursor_x	= win._mark_x_begin();
			_cursor_y0	= win._cursor_y0;
			_cursor_y1	= win._mark_y_begin();
		}
		else {
			_cursor_x	= win._cursor_x;
			_cursor_y0	= win._cursor_y0;
			_cursor_y1	= win._cursor_y1;
		}

	}


	auto& table = win._tune->table;
	auto& patterns = win._tune->patterns;
	auto& line = table[_cursor_y0];
	auto& pat_name = line[_cursor_x];
	auto pat = patterns.count(pat_name) ? &patterns[pat_name] : nullptr;
	auto row = (pat && _cursor_y1 < (int) pat->size()) ? &pat->at(_cursor_y1) : nullptr;


	switch (_type) {
	case SET_ROW:
	case RECORD_ROW:
		if (!row) return false;
		if (e == ECE_DO) _prev_row = *row;
		else _restore_cursor(win);
		if (e != ECE_UNDO) {
			if (*row == _row) return false;
			*row = _row;
		}
		else *row = _prev_row;
		return true;


	case SET_MACRO:
		if (!row) return false;
		if (e == ECE_DO) {
			_prev_row = *row;
			_row = *row;
			_row.macros[_index] = win._macro;
		}
		else _restore_cursor(win);
		if (e != ECE_UNDO) {
			if (*row == _row) return false;
			*row = _row;
		}
		else *row = _prev_row;
		return true;


	case DELETE_ROW:
		if (e == ECE_DO) {
			if (!pat || pat->size() <= 1) return false;
			_cursor_y1 = win._cursor_y1 = _index;
			if (_cursor_y1 >= (int) pat->size() - 1) {
				_cursor_y1 = win._cursor_y1 = pat->size() - 1;
			}
			_prev_row = pat->at(_cursor_y1);
		}
		else _restore_cursor(win);
		if (e != ECE_UNDO) {
			pat->erase(pat->begin() + _cursor_y1);
			if (_cursor_y1 > (int) pat->size() - 1) win._move_cursor(0, 0, -1);
		}
		else pat->insert(pat->begin() + _cursor_y1, _prev_row);
		return true;


	case INSERT_ROW:
		if (!pat) return false;
		if (e == ECE_DO) {
			if (_cursor_y1 >= (int) pat->size()) {
				_cursor_y1 = win._cursor_y1 = pat->size();
			}
		}
		else _restore_cursor(win);
		if (e != ECE_UNDO) {
			pat->insert(pat->begin() + _cursor_y1, Row());
		}
		else {
			pat->erase(pat->begin() + _cursor_y1);
			if (_cursor_y1 > (int) pat->size() - 1) win._move_cursor(0, 0, -1);
		}
		return true;



	case YANK_REGION:
		if (e == ECE_DO) {
			int xe = win._mark_x_end();
			int ye = win._mark_y_end();
			if (yank_region(*win._tune, _cursor_y0, _cursor_x, _cursor_y1, xe, ye,
				win._pattern_buffer, _clear)) return false;
			_prev_region = win._pattern_buffer;
			yank_region(*win._tune, _cursor_y0, _cursor_x, _cursor_y1, xe, ye, _region);
		}
		else _restore_cursor(win);
		if (e == ECE_REDO) paste_region(*win._tune, _cursor_y0, _cursor_x, _cursor_y1, _region);
		else if (e == ECE_UNDO) paste_region(*win._tune, _cursor_y0, _cursor_x, _cursor_y1, _prev_region);
		return true;


	case PASTE_REGION:
		if (e == ECE_DO) {
			_region = win._pattern_buffer;
			int len = 0;
			for (auto& b : _region) len = std::max(len, (int) b.size());
			yank_region(*win._tune, _cursor_y0, _cursor_x, _cursor_y1, _cursor_x + _region.size(),
				_cursor_y1 + len, _prev_region);
			if (paste_region(*win._tune, _cursor_y0, _cursor_x, _cursor_y1, _region, &_length_diffs)) {
				return false;
			}
		}
		else _restore_cursor(win);
		if (e == ECE_REDO) paste_region(*win._tune, _cursor_y0, _cursor_x, _cursor_y1, _region);
		else if (e == ECE_UNDO) {
			paste_region(*win._tune, _cursor_y0, _cursor_x, _cursor_y1, _prev_region);
			// trim patterns
			for (int c = 0; c < _region.size(); c++) {
				auto it = patterns.find(line[_cursor_x + c]);
				if (it != patterns.end()) {
					auto& pat = it->second;
					pat.resize(pat.size() - _length_diffs[c]);
				}
			}
		}
		return true;

	case TRANSPOSE_REGION:
		if (e == ECE_DO) {
			int xe = win._mark_x_end();
			int ye = win._mark_y_end();
			yank_region(*win._tune, _cursor_y0, _cursor_x, _cursor_y1, xe, ye, _prev_region);
			for (int c = _cursor_x; c < xe; c++) {
				auto it = patterns.find(line[c]);
				if (it != patterns.end()) {
					auto& pat = it->second;
					for (int i = _cursor_y1; i < ye && i < (int) pat.size(); i++) {
						auto& row = pat[i];
						if (row.note <= 0) continue;
						row.note = clamp(row.note + _index, 1, 120);
					}
				}
			}
			yank_region(*win._tune, _cursor_y0, _cursor_x, _cursor_y1, xe, ye, _region);
			if (_region == _prev_region) return false;
		}
		else _restore_cursor(win);
		if (e == ECE_REDO) paste_region(*win._tune, _cursor_y0, _cursor_x, _cursor_y1, _region);
		else if (e == ECE_UNDO) paste_region(*win._tune, _cursor_y0, _cursor_x, _cursor_y1, _prev_region);
		return true;


	case DELETE_LINE:
		if (e == ECE_DO) {
			if (table.size() <= 1) return false;
			if (_cursor_y0 >= (int) table.size() - 1) {
				_cursor_y0 = win._cursor_y0 = table.size() - 1;
			}
			_prev_line = line;
		}
		else _restore_cursor(win);
		if (e != ECE_UNDO) {
			table.erase(table.begin() + _cursor_y0);
			if (_cursor_y0 > (int) table.size() - 1) win._move_cursor(0, -1, 0);
		}
		else table.insert(table.begin() + _cursor_y0, _prev_line);
		return true;


	case INSERT_LINE:
		if (e == ECE_DO) {
			_cursor_y0 = win._cursor_y0 = _index;
			if (_cursor_y0 >= (int) table.size()) {
				_cursor_y0 = win._cursor_y0 = table.size();
			}
		}
		else _restore_cursor(win);
		if (e != ECE_UNDO) {
			TableLine l;
			table.insert(table.begin() + _cursor_y0, l);
		}
		else {
			table.erase(table.begin() + _cursor_y0);
			if (_cursor_y0 > (int) table.size() - 1) win._move_cursor(0, -1, 0);
		}
		return true;


	default:
		msg_win.say("Invalid command");
		return false;
	}
}



void PatternWin::resize() {
	_width = COLS - _left;
	_height = LINES - _top - 2;
	_scroll_x_view = (_width - 4) / (CHAN_CHAR_WIDTH + 1);

	_scroll_y0_view = 8;
	_scroll_y1_view = _height - _scroll_y0_view - 5;

	_scroll();
}

void PatternWin::_scroll() {
	if (_scroll_x > _cursor_x) _scroll_x = _cursor_x;
	if (_scroll_x < _cursor_x - _scroll_x_view + 1) {
		_scroll_x = _cursor_x - _scroll_x_view + 1;
	}
	if (_scroll_y0 > _cursor_y0) _scroll_y0 = _cursor_y0;
	if (_scroll_y0 < _cursor_y0 - _scroll_y0_view + 1) {
		_scroll_y0 = _cursor_y0 - _scroll_y0_view + 1;
	}
	if (_scroll_y1 > _cursor_y1) _scroll_y1 = _cursor_y1;
	if (_scroll_y1 < _cursor_y1 - _scroll_y1_view + 1) {
		_scroll_y1 = _cursor_y1 - _scroll_y1_view + 1;
	}
}

void PatternWin::_move_cursor(int dx, int dy0, int dy1) {
	if (dx) _cursor_x = (_cursor_x + dx + CHANNEL_COUNT) % CHANNEL_COUNT;
	if (dy0) _cursor_y0 = (_cursor_y0 + dy0 + _tune->table.size()) % _tune->table.size();
	if (dy1) {
		int max_rows = std::max(1, get_max_rows(*_tune, _cursor_y0));
		_cursor_y1 = (_cursor_y1 + dy1 + max_rows) % max_rows;
	}
	_scroll();
}

void PatternWin::draw() {
	int server_row = server.get_row();
	int server_line = server.get_line();

	// follow
	if (_edit_mode == EM_RECORD) {
		_cursor_y0 = server_line;
		_cursor_y1 = server_row;
		_scroll();
	}

	set_style(S_FRAME);
	mvprintw(_top, _left, "   ");
	addch(ACS_ULCORNER);
	mvprintw(_top + 1, _left, "   ");
	addch(ACS_VLINE);
	move(_top + 2, _left);
	addch(ACS_ULCORNER);
	addchs(ACS_HLINE, 2);
	addch(ACS_PLUS);
	for (int r = 0; r < _scroll_y0_view; r++) {
		move(_top + r + 3, _left);
		addch(ACS_VLINE);
		if (r + _scroll_y0 < (int) _tune->table.size()) {
			set_style(r == _cursor_y0 ? S_HL_NORMAL : S_NORMAL);
			printw("%02X", r + _scroll_y0);
			set_style(S_FRAME);
		}
		else printw("  ");
		set_style(S_FRAME);
		addch(ACS_VLINE);
	}



	auto& line = _tune->table[_cursor_y0];
	int max_rows = get_max_rows(*_tune, _cursor_y0);
	int y1 = _top + _scroll_y0_view + 3;

	move(y1, _left);
	addch(ACS_LTEE);
	addchs(ACS_HLINE, 2);
	addch(ACS_PLUS);
	for (int r = 0; r < _scroll_y1_view; r++) {
		int i = r + _scroll_y1;
		move(y1 + r + 1, _left);
		addch(ACS_VLINE);
		if (i < max_rows) {
			set_style(i == _cursor_y1 ? S_HL_NORMAL : S_NORMAL);
			printw("%02X", i);
			set_style(S_FRAME);
		}
		else printw("  ");
		set_style(S_FRAME);
		addch(ACS_VLINE);
	}

	move(_top + _height - 1, _left);
	addch(ACS_LLCORNER);
	addchs(ACS_HLINE, 2);
	addch(ACS_BTEE);


	int x = _left + 4;
	int chan_limit = std::min(_scroll_x + _scroll_x_view, (int) CHANNEL_COUNT);
	for (int chan_nr = _scroll_x; chan_nr < chan_limit; chan_nr++, x += CHAN_CHAR_WIDTH + 1) {

		auto& pat_name = line[chan_nr];
		auto pat = _tune->patterns.count(pat_name) ? &_tune->patterns[pat_name] : nullptr;


		// table head

		move(_top, x);
		addchs(ACS_HLINE, CHAN_CHAR_WIDTH);
		addch(chan_nr < chan_limit - 1 ? ACS_TTEE : ACS_URCORNER);
		move(_top + 1, x);


		int level = clamp(server.get_chan_level(chan_nr)) * (CHAN_CHAR_WIDTH - 1);
		for (int i = 0; i < CHAN_CHAR_WIDTH - 1; i++) {
			set_style(i < level ? S_LEVEL : S_NORMAL);
			int p = CHAN_CHAR_WIDTH / 2 - 3;
			addch(!server.get_muted(chan_nr) || i < p || i > p + 4 ? ' ' : "MUTED"[i - p]);
		}

		printw("%X", chan_nr);
		set_style(S_FRAME);
		addch(ACS_VLINE);
		move(_top + 2, x);
		addchs(ACS_HLINE, CHAN_CHAR_WIDTH);
		addch(chan_nr < chan_limit - 1 ? ACS_PLUS : ACS_RTEE);


		move(y1, x);
		addchs(ACS_HLINE, CHAN_CHAR_WIDTH);
		addch(chan_nr < chan_limit - 1 ? ACS_PLUS : ACS_RTEE);

		// top

		for (int r = 0; r < _scroll_y0_view; r++) {
			int i = r + _scroll_y0;
			move(_top + r + 3, x);
			if (i < (int) _tune->table.size()) {

				int style = S_PATTERN;
				if (i == server_line) style = S_PL_PATTERN;
				if (i == _cursor_y0) style = S_HL_PATTERN;
				if (i == _cursor_y0 && _cursor_x == chan_nr) {
					style = (_edit_mode == EM_PATTERN_NAME) ? S_ET_PATTERN : S_CS_PATTERN;
				}
				set_style(style);

				auto pn = _tune->table[i][chan_nr];
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
		for (int r = 0; r < _scroll_y1_view; r++) {
			int i = r + _scroll_y1;
			move(y1 + r + 1, x);

			bool on_pat = (pat && i < (int) pat->size());

			int style = on_pat ? S_NOTE : S_FRAME;
			if (on_pat && i == server_row && _tune->table[server_line][chan_nr] == pat_name) style = S_PL_NOTE;
			if (i == _cursor_y1) style = S_HL_NOTE;
			if (i == _cursor_y1 && _cursor_x == chan_nr) {
				style = (_edit_mode == EM_MACRO_NAME) ? S_ET_NOTE :
						(_edit_mode == EM_RECORD) ? S_RC_NOTE :
						S_CS_NOTE;
			}
			if (_edit_mode == EM_MARK_PATTERN
			&& _mark_x_begin() <= chan_nr && chan_nr < _mark_x_end()
			&& _mark_y_begin() <= i && i < _mark_y_end()) style = S_MK_NOTE;
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
					std::string _macro = row.macros[m];
					printw(" %s", _macro.c_str());
					addchs('.', MACRO_CHAR_WIDTH - _macro.size());
				}
			}
			else {
				addchs(' ', CHAN_CHAR_WIDTH);
			}
			set_style(S_FRAME);
			addch(ACS_VLINE);
		}

		set_style(S_FRAME);
		move(_top + _height - 1, x);
		addchs(ACS_HLINE, CHAN_CHAR_WIDTH);
		addch(chan_nr < chan_limit - 1 ? ACS_BTEE : ACS_LRCORNER);

	}
	set_style(S_DEFAULT);
	for (int r = 0; r < _height; r++) {
		move(_top + r, x);
		addchs(' ', std::max(0, _width - x));
	}



	// extra editing info
	set_style(S_FRAME);
	move(_top + _height - 1, _left);
	addch(ACS_LTEE);
	move(_top + _height - 1, _left + 3 + MACRO_CHAR_WIDTH);
	addch(ACS_TTEE);


	move(_top + _height, _left);
	addch(ACS_VLINE);
	set_style(S_NOTE);
	addch('0' + _octave);
	addch(' ');
	set_style(S_MACRO);
	printw("%s", _macro.c_str());
	addchs('.', MACRO_CHAR_WIDTH - _macro.size());
	set_style(S_FRAME);
	addch(ACS_VLINE);


	move(_top + _height + 1, _left);
	addch(ACS_LLCORNER);
	addchs(ACS_HLINE, 2 + MACRO_CHAR_WIDTH);
	addch(ACS_LRCORNER);

	set_style(S_NORMAL);
//	mvprintw(_top + _height + 1, _left, "|%1d|%-*s|", _octave, MACRO_CHAR_WIDTH, _macro.c_str());


	// set cursor position

	if (_edit_mode == EM_PATTERN_NAME) {
		curs_set(1);
		move(_top + 3 + _cursor_y0 - _scroll_y0,
			_left + (_cursor_x - _scroll_x) * (CHAN_CHAR_WIDTH + 1) + 4 + line[_cursor_x].size());
	}
	else if (_edit_mode == EM_MACRO_NAME) {
		curs_set(1);
		auto& pat_name = line[_cursor_x];
		auto& pat = _tune->patterns[pat_name];
		auto& row = pat[_cursor_y1];
		move(_top + 3 + _scroll_y0_view + _cursor_y1 - _scroll_y1 + 1,
			_left + (_cursor_x - _scroll_x) * (CHAN_CHAR_WIDTH + 1) + 4 + row.macros[0].size() + 4);
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
	switch (_edit_mode) {
	case EM_PATTERN_NAME:	_key_pattern_name(ch); break;
	case EM_MACRO_NAME:		_key_macro_name(ch); break;
	case EM_MARK_PATTERN:	_key_mark_pattern(ch); break;
	case EM_RECORD:			_key_record(ch); break;
	case EM_NORMAL:			_key_normal(ch); break;
	}
}

typedef EditCommand EC;

void PatternWin::_key_pattern_name(int ch) {
	auto& line = _tune->table[_cursor_y0];
	auto& pat_name = line[_cursor_x];
	if ((isalnum(ch) || strchr("_-+'\"*~!\\/#?$%&=<>", ch)) && pat_name.size() < PATTERN_CHAR_WIDTH) {
		pat_name += ch;
	}
	else if (ch == KEY_BACKSPACE && pat_name.size() > 0) pat_name.pop_back();
	else if (ch == KEY_ESCAPE) {
		_edit_mode = EM_NORMAL;
		pat_name.assign(_old_name);
	}
	else if (ch == '\n') {
		_edit_mode = EM_NORMAL;

		if (pat_name != "" && _tune->patterns.count(pat_name) == 0) {
			if (_rename_pattern && _old_name != "") { // rename pattern
				_tune->patterns[pat_name] = _tune->patterns[_old_name];
			}
			else {
				// new pattern
				int len = std::max<int>(1, get_max_rows(*_tune, _cursor_y0));
				_tune->patterns[pat_name].resize(len);
			}
		}
	}
}

void PatternWin::_key_macro_name(int ch) {
	auto& line = _tune->table[_cursor_y0];
	auto& pat_name = line[_cursor_x];
	auto& pat = _tune->patterns[pat_name];
	auto& row = pat[_cursor_y1];
	auto& macro_name = row.macros[0];

	if ((isalnum(ch) || strchr("_-+'\"*~!\\/#?$%&=<>", ch)) && macro_name.size() < MACRO_CHAR_WIDTH) {
		macro_name += ch;
	}
	else if (ch == KEY_BACKSPACE && macro_name.size() > 0) macro_name.pop_back();
	else if (ch == KEY_ESCAPE) {
		_edit_mode = EM_NORMAL;
		macro_name.assign(_old_name);
	}
	else if (ch == '\n') {
		_edit_mode = EM_NORMAL;
		_macro = macro_name;
		macro_name.assign(_old_name);
		_edit<EC::SET_MACRO>(0);
	}
}

void PatternWin::_key_mark_pattern(int ch) {
	switch (ch) {
	case KEY_UP:	_move_cursor( 0, 0, -1); return;
	case KEY_DOWN:	_move_cursor( 0, 0,  1); return;
	case KEY_PPAGE:	_move_cursor( 0, 0, -4); return;
	case KEY_NPAGE:	_move_cursor( 0, 0,  4); return;
	case KEY_RIGHT:	_move_cursor( 1, 0,  0); return;
	case KEY_LEFT:	_move_cursor(-1, 0,  0); return;

	// mark whole pattern
	case 'V':
		_mark_y = 0;
		_cursor_y1 = std::max<int>(0, get_max_rows(*_tune, _cursor_y0) - 1);
		_scroll();
		return;

	case KEY_ESCAPE:
		_edit_mode = EM_NORMAL;
		return;

	// copy pattern to buffer
	case 'y':
	case 'd':
	case KEY_BACKSPACE:
		_edit_mode = EM_NORMAL;
		_edit<EC::YANK_REGION>(ch != 'y');
		return;

	// transpose
	case '>':
	case '<':
		_edit<EC::TRANSPOSE_REGION>((ch == '>') - (ch == '<'));
		return;

	default: break;
	}
}

void PatternWin::_key_rec_norm_common(int ch) {
	switch (ch) {
	case KEY_CTRL_RIGHT:
	case KEY_RIGHT:		_move_cursor(1, 0, 0); return;
	case KEY_LEFT:
	case KEY_CTRL_LEFT:	_move_cursor(-1, 0, 0); return;

	case '<':
		if(--_octave < 0) _octave = 0;
		return;
	case '>':
		if (++_octave > 8) _octave = 8;
		return;

	case '+': {
			auto it = _tune->macros.find(_macro);
			it++;
			if (_macro == "") it = _tune->macros.begin();
			if (it != _tune->macros.end()) _macro = it->first;
			else _macro = "";
		}
		return;
	case '-': {
			MacroMap::reverse_iterator it(_tune->macros.find(_macro));
			if (_macro == "") it = _tune->macros.rbegin();
			if (it != _tune->macros.rend()) _macro = it->first;
			else _macro = "";
		}
		return;

	case '\0':	// continue playing
		if (server.is_playing()) server.pause();
		else server.play(-1);
		return;

	case ' ':	// play from the begining current line
		if (server.is_playing()) server.pause();
		else server.play(_cursor_y0);
		return;

	case '\n':	// loop current line
		if (server.is_playing()) server.pause();
		else server.play(_cursor_y0, true);
		return;


	case 'M':	// mute
		server.set_muted(_cursor_x, !server.get_muted(_cursor_x));
		if (server.get_muted(_cursor_x)) server.play_row(_cursor_x, { -1 });
		return;
	case 'L':	// solo
		{
			int s = 0;
			for (int i = 0; i < CHANNEL_COUNT; i++) s += server.get_muted(i);
			if (!server.get_muted(_cursor_x) && s == CHANNEL_COUNT - 1) {
				for (int i = 0; i < CHANNEL_COUNT; i++) server.set_muted(i, false);
			}
			else {
				for (int i = 0; i < CHANNEL_COUNT; i++) {
					server.set_muted(i, i != _cursor_x);
					if (i != _cursor_x) server.play_row(i, { -1 });
				}
			}
		}
		return;

	default: break;
	}
}

void PatternWin::_key_record(int ch) {

	_key_rec_norm_common(ch);
	if (ch == KEY_TAB) {
		_edit_mode = EM_NORMAL;
		return;
	}

	if (ch < 32 || ch > 127) return;
	if (ch == '^') {
		Row row { -1 };
		_edit<EC::RECORD_ROW>(row);
		server.play_row(_cursor_x, row);
		return;
	}
	static const char* t1 = "ysxdcvgbhnjm,";
	static const char* t2 = "q2w3er5t6z7ui";
	const char* a = nullptr;
	int n;
	if ((a = strchr(t1, ch))) n = a - t1;
	else if ((a = strchr(t2, ch))) n = a - t2 + 12;
	if (a) {
		Row row { n + 1 + _octave * 12 };
		row.macros[0] = _macro;
		_edit<EC::RECORD_ROW>(row);
		server.play_row(_cursor_x, row);
	}
}

void PatternWin::_key_normal(int ch) {
	auto& line = _tune->table[_cursor_y0];
	auto& pat_name = line[_cursor_x];
	auto pat = _tune->patterns.count(pat_name) ? &_tune->patterns[pat_name] : nullptr;
	auto row = (pat && _cursor_y1 < (int) pat->size()) ? &pat->at(_cursor_y1) : nullptr;

	_key_rec_norm_common(ch);

	switch (ch) {
	case KEY_UP:		_move_cursor(0,  0, -1); return;
	case KEY_DOWN:		_move_cursor(0,  0,  1); return;
	case KEY_PPAGE:		_move_cursor(0,  0, -4); return;
	case KEY_NPAGE:		_move_cursor(0,  0,  4); return;
	case KEY_CTRL_UP:	_move_cursor(0, -1,  0); return;
	case KEY_CTRL_DOWN:	_move_cursor(0,  1,  0); return;

	case KEY_BACKSPACE:
		_edit<EC::SET_ROW>(Row());
		return;

	case '!':			// set 1st macro
		_edit<EC::SET_MACRO>(0);
		return;
	case '"':			// set 2nd macro
		_edit<EC::SET_MACRO>(1);
		return;
	case 'I':			// edit _macro name
		if (row) {
			_edit_mode = EM_MACRO_NAME;
			_old_name = row->macros[0];
		}
		return;

	case 'V':			// mark pattern
		_edit_mode = EM_MARK_PATTERN;
		_mark_x = _cursor_x;
		_mark_y = _cursor_y1;
		return;

	case 'P':			// paste pattern
		_edit<EC::PASTE_REGION>();
		return;

	case 'X':			// delete row
		_edit<EC::DELETE_ROW>();
		return;

	case 'O':			// insert new row
		_edit<EC::INSERT_ROW>(_cursor_y1);
		return;

	case 'A':			// append new row
		_edit<EC::INSERT_ROW>(_cursor_y1 + 1);
		_scroll();
		return;

	case KEY_CTRL_X:	// delete line
		_edit<EC::DELETE_LINE>();
		return;

	case KEY_CTRL_O:	// insert new line
		_edit<EC::INSERT_LINE>(_cursor_y0);
		return;

	case KEY_CTRL_A:	// append new line
		_edit<EC::INSERT_LINE>(_cursor_y0 + 1);
		_scroll();
		return;

	case KEY_CTRL_R:	// rename pattern
		_edit_mode = EM_PATTERN_NAME;
		_rename_pattern = true;
		_old_name = pat_name;
		return;

	case KEY_CTRL_N:	// new pattern
		_edit_mode = EM_PATTERN_NAME;
		_rename_pattern = false;
		_old_name = pat_name;
		return;

	case KEY_CTRL_D: 	// duplicate above pattern with auto naming
		if (pat) return;
		for (int i = _cursor_y0; i >= 0; i--) {
			auto pn = _tune->table[i][_cursor_x];
			if (pn != "") {
				auto& p = _tune->patterns[pn];
				auto pos = pn.find_last_not_of("0123456789");
				auto suffix = pn.substr(pos + 1);
				if (suffix == "") suffix = "0";
				else pn.erase(pos + 1);
				while (_tune->patterns.find(pn + suffix) != _tune->patterns.end()) {
					auto n = std::to_string(std::stoi(suffix) + 1);
					suffix = std::string(std::max<int>(0, suffix.size() - n.size()), '0') + n;
				}
				pn += suffix;
				_tune->table[_cursor_y0][_cursor_x] = pn;
				_tune->patterns[pn] = p;
				return;
			}
		}
		return;

	case KEY_TAB:
		_edit_mode = EM_RECORD;
		return;

	case 'S':
		strip_tune(*_tune);
		msg_win.say("Saving tune file... ");
		if (!save_tune(*_tune, _tunefile)) msg_win.append("error.");
		else msg_win.append("done.");
		return;

	case 'U':
		_undo();
		return;
	case 'R':
		_redo();
		return;


	default: break;
	}


	if (ch < 32 || ch > 127) return;
	if (ch == '^') {
		Row row { -1 };
		_edit<EC::SET_ROW>(row);
		server.play_row(_cursor_x, row);
		return;
	}
	static const char* t1 = "ysxdcvgbhnjm,";
	static const char* t2 = "q2w3er5t6z7ui";
	const char* a = nullptr;
	int n;
	if ((a = strchr(t1, ch))) n = a - t1;
	else if ((a = strchr(t2, ch))) n = a - t2 + 12;
	if (a) {
		Row row { n + 1 + _octave * 12 };
		row.macros[0] = _macro;
		_edit<EC::SET_ROW>(row);
		server.play_row(_cursor_x, row);
	}
}

void PatternWin::midi(int type, int value) {

	Row row;
	int chan;

	if (type == 128) { // note off event
		if (_note_to_chan[value] == -1) return;
		chan = _note_to_chan[value];
		_chan_to_note[chan] = -1;
		_note_to_chan[value] = -1;
		row.note = -1;
	}
	else if (type == 144) {

		chan = _cursor_x;
		for (int i = 1; i < POLYPHONY; i++) {
			if (_chan_to_note[chan] == -1) break;
			chan = (chan + 1) % CHANNEL_COUNT;
		}
		int old_note = _chan_to_note[chan];
		if (old_note != -1) _note_to_chan[old_note] = -1;
		_chan_to_note[chan] = value;
		_note_to_chan[value] = chan;

		row.note = value + 1;
		row.macros[0] = _macro;
	}
	else return;

	server.play_row(chan, row);

	if (row.note > 0 && _edit_mode == EM_NORMAL) {
		_edit<EC::SET_ROW>(row);
	}
	else if (_edit_mode == EM_RECORD) {

		// record note off event only if no other voice active
		if (row.note == -1) {
			for (int n : _chan_to_note) if (n != -1) return;
		}

		_edit<EC::RECORD_ROW>(row);
	}
}
