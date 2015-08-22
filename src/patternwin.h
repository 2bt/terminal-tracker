#pragma once

#include "win.h"
#include "tune.h"

class PatternWin : public Win {
public:
	enum {
		PATTERN_CHAR_WIDTH = 10,
		MACRO_CHAR_WIDTH = 5,
		CHAN_CHAR_WIDTH = 3 + MACROS_PER_ROW * (MACRO_CHAR_WIDTH + 1)
	};

	void init(Tune* tune, const char* tunefile) {
		this->tune = tune;
		this->tunefile = tunefile;
		resize();
	}

	virtual void resize();
	virtual void draw();
	virtual void key(int ch);


private:
	void do_scroll();
	void move_cursor(int dx, int dy0, int dy1);


	int top = 1;
	int left = 0;
	int width;
	int height;

	int cursor_x = 0;
	int cursor_y0 = 0;
	int cursor_y1 = 0;
	int mark_x;
	int mark_y;
	int scroll_x = 0;
	int scroll_y0 = 0;
	int scroll_y1 = 0;
	int scroll_x_view;
	int scroll_y0_view;
	int scroll_y1_view;


	enum EditMode { OFF, PATTERN, MACRO, MARK_BLOCK, MARK_PATTERN };
	EditMode		edit_mode = OFF;
	std::string		old_name;

	Pattern			buffer_pat;

	std::string		macro = "";
	int				octave = 3;

	Tune*			tune;
	const char*		tunefile;
};
