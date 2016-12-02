#pragma once

#include <cmath>
#include "param.h"
#include "fx.h"


template <typename T>
T clamp(const T& n, const T& a=0.0f, const T& b=1.0f) {
	return std::max(a, std::min(n, b));
}


class Channel {
public:
	Channel();

	void init();
	void reset_params();
	void configure_params(const EnvelopeMap& envs);
	void note_event(int note);
	void tick();
	void add_mix(float* frame, const Channel& modulator, FX& fx);

	float get_level() const { return m_level * m_volume; }

private:
	bool set_param_env(std::string name, const Envelope& env);

	enum class State { OFF, RELEASE, ATTACK, HOLD };
	enum class Wave { PULSE, TRIANGLE, SINE, NOISE, C64NOISE };

	State			m_state;
	float			m_level;
	float			m_phase;
	float			m_speed;
	unsigned int	m_shift = 0x7ffff8;


	float			m_note;
	float			m_dst_note;
	float			m_gliss;
	float			m_offset;

	Wave			m_wave;
	float			m_pulsewidth;
	float			m_pulsewidth_sweep;
	float			m_volume;
	float			m_panning[2];
	float			m_smooth[2];

	float			m_resolution;
	float			m_vibrato_phase;
	float			m_vibrato_speed;
	float			m_vibrato_depth;

	float			m_attack;
	float			m_decay;
	float			m_sustain;
	float			m_release;

	bool			m_sync;
	float			m_amp;
	float			m_ringmod;

	uint32_t		m_filter;
	float			m_cutoff;
	float			m_resonance;
	float			m_filter_l;
	float			m_filter_b;
	float			m_filter_h;

	float			m_echo;

	enum ParamID {
		PID_WAVE,
		PID_OFFSET,
		PID_GLISS,
		PID_VOLUME,
		PID_PANNING,
		PID_PULSEWIDTH,
		PID_PULSEWIDTH_SWEEP,
		PID_RESOLUTION,
		PID_VIBRATO_SPEED,
		PID_VIBRATO_DEPTH,
		PID_ATTACK,
		PID_DECAY,
		PID_SUSTAIN,
		PID_RELEASE,
		PID_SYNC,
		PID_RINGMOD,
		PID_FILTER,
		PID_CUTOFF,
		PID_RESONANCE,
		PID_ECHO,

		PID_PARAM_COUNT
	};

	static const std::map<std::string,int>		m_param_mapping;
	ParamBatch<PID_PARAM_COUNT,m_param_mapping>	m_param_batch;
};

