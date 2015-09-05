#pragma once

#include <vector>
#include <string>
#include <chrono>
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

	virtual void draw();
	void append(const char* format, ...);
	void say(const char* format, ...);

private:

	int top;
	int left = 0;
	int width;
	int height = MAX_MESSAGES;

	struct Msg {
		std::string text;
		std::chrono::time_point<std::chrono::system_clock> time;
	};
	std::vector<Msg> messages;
};

extern MessageWin msg_win;
