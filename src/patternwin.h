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

	PatternWin(Tune& tune) : tune(tune) {
		top = 1;
		left = 0;
		resize();
	}
	virtual void resize();
	virtual void draw();
	virtual void key(int ch);


private:
	void do_scroll();


	int top = 0;
	int left = 0;
	int width;
	int height;

	int cursor_x = 0;
	int cursor_y0 = 0;
	int cursor_y1 = 0;
	int scroll_x = 0;
	int scroll_y0 = 0;
	int scroll_y1 = 0;
	int scroll_x_view;
	int scroll_y0_view;
	int scroll_y1_view;


	bool			edit_name = false;
	std::string		old_name;


	std::string		macro = "foo";
	int				octave = 3;

	Tune& tune;
};
