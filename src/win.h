#pragma once

class Win {
public:
	Win() {}
	virtual ~Win() {}
	virtual void draw() = 0;
	virtual void key(int ch) {}
	virtual void resize() {}
};
