#pragma once

#include "win.h"
#include "tune.h"


class PatternWin;

class EditCommand {
public:
	enum Execution { ECE_DO, ECE_UNDO, ECE_REDO };
	enum Type {
		SET_NOTE,
		SET_MACRO,
		SET_ROW,
		RECORD_ROW,
		YANK_REGION,
		PASTE_REGION,
		TRANSPOSE_REGION,
		DELETE_ROW,
		INSERT_ROW,
		DELETE_LINE,
		INSERT_LINE,
		// TODO:
		// pattern renaming
	};


	template <Type type>
	void init() {
		_type = type;
	}

	template <Type type>
	void init(const Row& row) {
		_type = type;
		_row = row;
	}

	template <Type type>
	void init(bool clear) {
		_type = type;
		_clear = clear;
	}

	template <Type type>
	void init(int index) {
		_type = type;
		_index = index;
	}

	bool exec(PatternWin& win, Execution e=ECE_DO);

private:
	void _restore_cursor(PatternWin& win) const;

	Type	_type;
	int		_cursor_x;
	int		_cursor_y0;
	int		_cursor_y1;
	Row		_row;
	Row		_prev_row;
	int		_index;
	int		_clear;
	std::vector<Pattern>	_region;
	std::vector<Pattern>	_prev_region;
	std::vector<int>		_length_diffs;
	TableLine				_prev_line;
};



class PatternWin : public Win {
	friend class EditCommand;
public:
	enum {
		PATTERN_CHAR_WIDTH = 9,
		MACRO_CHAR_WIDTH = 5,
		CHAN_CHAR_WIDTH = 3 + MACROS_PER_ROW * (MACRO_CHAR_WIDTH + 1),

		POLYPHONY = 5
	};


	void init(Tune* tune, const char* tunefile) {
		for (auto& c : _note_to_chan) c = -1;
		for (auto& n : _chan_to_note) n = -1;
		_tune = tune;
		_tunefile = tunefile;
		resize();
	}

	virtual void resize();
	virtual void draw();
	virtual void key(int ch);
	void midi(int type, int value);

private:

	int	_top = 0;
	int	_left = 0;
	int	_width;
	int	_height;

	int	_cursor_x = 0;
	int	_cursor_y0 = 0;
	int	_cursor_y1 = 0;
	int	_mark_x;
	int	_mark_y;

	int	_scroll_x = 0;
	int	_scroll_y0 = 0;
	int	_scroll_y1 = 0;
	int	_scroll_x_view;
	int	_scroll_y0_view;
	int	_scroll_y1_view;

	int	_mark_y_begin() const { return std::min(_cursor_y1, _mark_y); }
	int	_mark_y_end() const { return std::max(_cursor_y1, _mark_y) + 1; }
	int	_mark_x_begin() const { return std::min(_cursor_x, _mark_x); }
	int	_mark_x_end() const { return std::max(_cursor_x, _mark_x) + 1; }

	void _scroll();
	void _move_cursor(int dx, int dy0, int dy1);

	void _key_pattern_name(int ch);
	void _key_macro_name(int ch);
	void _key_mark_pattern(int ch);
	void _key_record(int ch);
	void _key_normal(int ch);
	void _key_rec_norm_common(int ch);

	enum EditMode { EM_NORMAL, EM_RECORD, EM_PATTERN_NAME, EM_MACRO_NAME, EM_MARK_PATTERN };
	EditMode				_edit_mode = EM_NORMAL;
	bool					_rename_pattern;
	std::string				_old_name;
	std::vector<Pattern>	_pattern_buffer;
	std::string				_macro;
	int						_octave = 3;
	Tune*					_tune;
	const char*				_tunefile;

	// midi keyboard state
	int				_note_to_chan[128];
	int				_chan_to_note[CHANNEL_COUNT];


	std::array<EditCommand,1024>	_cmds;
	int _cmd_head = 0;
	int _cmd_tail = 0;
	int _cmd_index = 0;

	template <EditCommand::Type type, typename... Args>
	void _edit(Args&&... args) {
		_cmds[_cmd_index].init<type>(args...);

		if (!_cmds[_cmd_index].exec(*this)) return;
		_cmd_index = (_cmd_index + 1) % _cmds.size();

		if (_cmd_head == _cmd_index) {
			_cmd_head = (_cmd_head + 1) % _cmds.size();
		}
		_cmd_tail = _cmd_index;
	}
	void _undo() {
		if (_cmd_index == _cmd_head) return;
		_cmd_index = (_cmd_index + _cmds.size() - 1) % _cmds.size();
		_cmds[_cmd_index].exec(*this, EditCommand::ECE_UNDO);
	}
	void _redo() {
		if (_cmd_index == _cmd_tail) return;
		_cmds[_cmd_index].exec(*this, EditCommand::ECE_REDO);
		_cmd_index = (_cmd_index + 1) % _cmds.size();
	}

};
