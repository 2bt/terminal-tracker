#pragma once

#include "tune.h"

class FX {
	friend class Server;
public:

	void init();
	void echo(float l, float r);
	void add_mix(float* frame);

private:
	enum { MAX_ECHO_LENGTH = MIXRATE * 2 };


	float	_echo_frames[MAX_ECHO_LENGTH][2];
	int		_echo_pos;

	float	_echo_feedback;
	int		_echo_length;
};

