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
	Channel() { init(); }

	void init();
	bool set_param_envs(const EnvelopeMap& envs);
	void note_event(int note);
	void tick();
	void add_mix(float* frame, const Channel& modulator, FX& fx);

	float get_level() const { return _level * _volume; }

private:
	bool set_param_env(std::string name, const Envelope& env);

	enum class State { OFF, RELEASE, ATTACK, HOLD };
	enum class Wave { PULSE, TRIANGLE, SINE, NOISE, C64NOISE };

	State			_state;
	float			_level;
	float			_phase;
	float			_speed;
	unsigned int	_shift = 0x7ffff8;


	float			_note;
	float			_dst_note;
	float			_gliss;
	float			_offset;

	Wave			_wave;
	float			_pulsewidth;
	float			_pulsewidth_sweep;
	float			_volume;
	float			_panning[2];
	float			_smooth[2];

	float			_resolution;
	float			_vibrato_phase;
	float			_vibrato_speed;
	float			_vibrato_depth;

	float			_attack;
	float			_decay;
	float			_sustain;
	float			_release;

	bool			_sync;
	float			_amp;
	float			_ringmod;

	uint32_t		_filter;
	float			_cutoff;
	float			_resonance;
	float			_filter_l;
	float			_filter_b;
	float			_filter_h;

	float			_echo;


	enum class ParamID {
		WAVE,
		OFFSET,
		GLISS,
		VOLUME,
		PANNING,
		PULSEWIDTH,
		PULSEWIDTH_SWEEP,
		RESOLUTION,
		VIBRATO_SPEED,
		VIBRATO_DEPTH,

		ATTACK,
		DECAY,
		SUSTAIN,
		RELEASE,

		SYNC,
		RINGMOD,

		FILTER,
		CUTOFF,
		RESONANCE,

		ECHO,

		PARAM_COUNT
	};

	std::array<Param,(int)ParamID::PARAM_COUNT> _params;
	void param_change(ParamID id, float v);
};

