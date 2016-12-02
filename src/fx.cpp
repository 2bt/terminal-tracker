#include <string.h>
#include "fx.h"


void FX::init() {

	memset(m_echo_frames, 0, sizeof(m_echo_frames));
	m_echo_pos = 0;

	m_echo_feedback = 0.4;
	m_echo_length = 650 * 8 * 3;
}


void FX::echo(float l, float r) {
	m_echo_frames[m_echo_pos][0] += l;
	m_echo_frames[m_echo_pos][1] += r;
}


void FX::add_mix(float* frame) {

	float* echo = m_echo_frames[(m_echo_pos + MAX_ECHO_LENGTH - m_echo_length) % MAX_ECHO_LENGTH];

	frame[0] += echo[1] * m_echo_feedback;
	frame[1] += echo[0] * m_echo_feedback;

	m_echo_frames[m_echo_pos][0] += echo[1] * m_echo_feedback;
	m_echo_frames[m_echo_pos][1] += echo[0] * m_echo_feedback;

	m_echo_pos = (m_echo_pos + 1) % MAX_ECHO_LENGTH;
	m_echo_frames[m_echo_pos][0] = 0;
	m_echo_frames[m_echo_pos][1] = 0;
}
