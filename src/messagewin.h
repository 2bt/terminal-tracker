#pragma once

#include <vector>
#include <string>
#include "win.h"
#include "styles.h"


class MessageWin : public Win {
public:
	enum { MAX_MESSAGES = 2 };

	MessageWin() {
		resize();
	}

	virtual void resize() {
		top = LINES - MAX_MESSAGES;
		width = COLS;
	}

	virtual void draw() {
		for (int y = 0; y < height; y++) {
			move(top + height - y - 1, left);
			if (y < (int) messages.size()) {
				printw("%-*s", width, messages[y].c_str());
			}
			else {
				printw("%-*s", width, "");
			}
		}
	}

	void append(const char* format, ...) {
		char line[256];
		va_list a;
		va_start(a, format);
		vsnprintf(line, 256, format, a);
		va_end(a);
		messages.front() += line;
	}
	void say(const char* format, ...) {
		char line[256];
		va_list a;
		va_start(a, format);
		vsnprintf(line, 256, format, a);
		va_end(a);
		messages.insert(messages.begin(), line);
		while (messages.size() > MAX_MESSAGES) messages.pop_back();
	}

private:

	int top;
	int left = 0;
	int width;
	int height = MAX_MESSAGES;

	std::vector<std::string> messages;
};

extern MessageWin msg_win;
