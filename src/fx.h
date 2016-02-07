#pragma once

#include "tune.h"

class FX {
public:

	void init();
	void echo(float l, float r);
	void add_mix(float* frame);

private:
	enum { MAX_ECHO_LENGTH = MIXRATE * 2 };


	float	_echo_buffers[MAX_ECHO_LENGTH][2];
	int		_echo_pos;

	float	_echo_feedback;
	float	_echo_gain;
	int		_echo_length;
};

