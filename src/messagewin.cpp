#include <ncurses.h>
#include "messagewin.h"


void MessageWin::draw() {
	for (int y = 0; y < height; y++) {
		move(top + y, left);
		if (y < (int) messages.size()) {
			printw("%-*s", width, messages[y].text.c_str());
		}
		else {
			printw("%-*s", width, "");
		}
	}
	auto now = std::chrono::system_clock::now();
	while (!messages.empty() && messages.front().time + std::chrono::seconds(1) < now) {
		messages.erase(messages.begin());
	}
}

void MessageWin::append(const char* format, ...) {
	char line[256];
	va_list a;
	va_start(a, format);
	vsnprintf(line, 256, format, a);
	va_end(a);
	messages.back().text += line;
}
void MessageWin::say(const char* format, ...) {
	char line[256];
	va_list a;
	va_start(a, format);
	vsnprintf(line, 256, format, a);
	va_end(a);
	messages.push_back({ line, std::chrono::system_clock::now() });
	while (messages.size() > MAX_MESSAGES) messages.erase(messages.begin());

}
