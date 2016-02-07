#include <string.h>
#include "fx.h"


void FX::init() {

	memset(_echo_frames, 0, sizeof(_echo_frames));
	_echo_pos = 0;

	_echo_feedback = 0.4;
	_echo_length = 650 * 8 * 3;
}


void FX::echo(float l, float r) {
	_echo_frames[_echo_pos][0] += l;
	_echo_frames[_echo_pos][1] += r;
}


void FX::add_mix(float* frame) {

	float* echo = _echo_frames[(_echo_pos + MAX_ECHO_LENGTH - _echo_length) % MAX_ECHO_LENGTH];

	frame[0] += echo[1] * _echo_feedback;
	frame[1] += echo[0] * _echo_feedback;

	_echo_frames[_echo_pos][0] += echo[1] * _echo_feedback;
	_echo_frames[_echo_pos][1] += echo[0] * _echo_feedback;

	_echo_pos = (_echo_pos + 1) % MAX_ECHO_LENGTH;
	_echo_frames[_echo_pos][0] = 0;
	_echo_frames[_echo_pos][1] = 0;
}
