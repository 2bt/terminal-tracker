#include <string.h>
#include "fx.h"


void FX::init() {

	// echo
	memset(_echo_buffers, 0, sizeof(_echo_buffers));
	_echo_pos = 0;

	_echo_feedback = 0.5;
	_echo_length = 10000;
	_echo_gain = 0.5;

}


void FX::echo(float l, float r) {
	_echo_buffers[_echo_pos][0] += l;
	_echo_buffers[_echo_pos][1] += r;
}


void FX::add_mix(float* frame) {

	int p = (_echo_pos + MAX_ECHO_LENGTH - _echo_length) % MAX_ECHO_LENGTH;

	_echo_buffers[_echo_pos][0] += _echo_buffers[p][0] * _echo_feedback;
	_echo_buffers[_echo_pos][1] += _echo_buffers[p][1] * _echo_feedback;

	frame[0] += _echo_buffers[p][0] * _echo_gain;
	frame[1] += _echo_buffers[p][1] * _echo_gain;


	_echo_pos = (_echo_pos + 1) % MAX_ECHO_LENGTH;
	_echo_buffers[_echo_pos][0] = 0;
	_echo_buffers[_echo_pos][1] = 0;
}
