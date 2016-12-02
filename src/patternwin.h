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
		m_type = type;
	}

	template <Type type>
	void init(const Row& row) {
		m_type = type;
		m_row = row;
	}

	template <Type type>
	void init(bool clear) {
		m_type = type;
		m_clear = clear;
	}

	template <Type type>
	void init(int index) {
		m_type = type;
		m_index = index;
	}

	bool exec(PatternWin& win, Execution e=ECE_DO);

private:
	void m_restore_cursor(PatternWin& win) const;

	Type	m_type;
	int		m_cursor_x;
	int		m_cursor_y0;
	int		m_cursor_y1;
	Row		m_row;
	Row		m_prev_row;
	int		m_index;
	int		m_clear;
	std::vector<Pattern>	m_region;
	std::vector<Pattern>	m_prev_region;
	std::vector<int>		m_length_diffs;
	TableLine				m_prev_line;
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
		for (auto& c : m_note_to_chan) c = -1;
		for (auto& n : m_chan_to_note) n = -1;
		m_tune = tune;
		m_tunefile = tunefile;
		resize();
	}

	virtual void resize();
	virtual void draw();
	virtual void key(int ch);
	void midi(int type, int value);

private:

	int	m_top = 0;
	int	m_left = 0;
	int	m_width;
	int	m_height;

	int	m_cursor_x = 0;
	int	m_cursor_y0 = 0;
	int	m_cursor_y1 = 0;
	int	m_mark_x;
	int	m_mark_y;

	int	m_scroll_x = 0;
	int	m_scroll_y0 = 0;
	int	m_scroll_y1 = 0;
	int	m_scroll_x_view;
	int	m_scroll_y0_view;
	int	m_scroll_y1_view;

	int	m_mark_y_begin() const { return std::min(m_cursor_y1, m_mark_y); }
	int	m_mark_y_end() const { return std::max(m_cursor_y1, m_mark_y) + 1; }
	int	m_mark_x_begin() const { return std::min(m_cursor_x, m_mark_x); }
	int	m_mark_x_end() const { return std::max(m_cursor_x, m_mark_x) + 1; }

	void m_scroll();
	void m_move_cursor(int dx, int dy0, int dy1);

	void m_key_pattern_name(int ch);
	void m_key_macro_name(int ch);
	void m_key_mark_pattern(int ch);
	void m_key_record(int ch);
	void m_key_normal(int ch);
	void m_key_rec_norm_common(int ch);

	enum EditMode { EM_NORMAL, EM_RECORD, EM_PATTERN_NAME, EM_MACRO_NAME, EM_MARK_PATTERN };
	EditMode				m_edit_mode = EM_NORMAL;
	bool					m_rename_pattern;
	std::string				m_old_name;
	std::vector<Pattern>	m_pattern_buffer;
	std::string				m_macro;
	int						m_octave = 3;
	Tune*					m_tune;
	const char*				m_tunefile;

	// midi keyboard state
	int				m_note_to_chan[128];
	int				m_chan_to_note[CHANNEL_COUNT];


	std::array<EditCommand,1024>	m_cmds;
	int m_cmd_head = 0;
	int m_cmd_tail = 0;
	int m_cmd_index = 0;

	template <EditCommand::Type type, typename... Args>
	void m_edit(Args&&... args) {
		m_cmds[m_cmd_index].init<type>(args...);

		if (!m_cmds[m_cmd_index].exec(*this)) return;
		m_cmd_index = (m_cmd_index + 1) % m_cmds.size();

		if (m_cmd_head == m_cmd_index) {
			m_cmd_head = (m_cmd_head + 1) % m_cmds.size();
		}
		m_cmd_tail = m_cmd_index;
	}
	void m_undo() {
		if (m_cmd_index == m_cmd_head) return;
		m_cmd_index = (m_cmd_index + m_cmds.size() - 1) % m_cmds.size();
		m_cmds[m_cmd_index].exec(*this, EditCommand::ECE_UNDO);
	}
	void m_redo() {
		if (m_cmd_index == m_cmd_tail) return;
		m_cmds[m_cmd_index].exec(*this, EditCommand::ECE_REDO);
		m_cmd_index = (m_cmd_index + 1) % m_cmds.size();
	}

};
