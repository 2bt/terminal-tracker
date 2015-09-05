#include <ncurses.h>
#include "messagewin.h"


void MessageWin::draw() {
	for (int y = 0; y < height; y++) {
		move(top + height - y - 1, left);
		if (y < (int) messages.size()) {
			printw("%-*s", width, messages[y].text.c_str());
		}
		else {
			printw("%-*s", width, "");
		}
	}
	auto now = std::chrono::system_clock::now();
	while (!messages.empty() && messages.back().time + std::chrono::seconds(1) < now) {
		messages.pop_back();
	}
}

void MessageWin::append(const char* format, ...) {
	char line[256];
	va_list a;
	va_start(a, format);
	vsnprintf(line, 256, format, a);
	va_end(a);
	messages.front().text += line;
}
void MessageWin::say(const char* format, ...) {
	char line[256];
	va_list a;
	va_start(a, format);
	vsnprintf(line, 256, format, a);
	va_end(a);
	messages.insert(messages.begin(), { line, std::chrono::system_clock::now() });
	while (messages.size() > MAX_MESSAGES) messages.pop_back();
}
