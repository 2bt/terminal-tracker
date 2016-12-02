#pragma once

#include "tune.h"


class FX {
	friend class Server;
public:

	void init();
	void echo(float l, float r);
	void add_mix(float* frame);

	enum {
		MAX_ECHO_LENGTH = MIXRATE * 2
	};


private:

	float	m_echo_frames[MAX_ECHO_LENGTH][2];
	int		m_echo_pos;


	float	m_echo_feedback;
	int		m_echo_length;
};
