#pragma once

#include <cmath>
#include "param.h"


template <typename T>
T clamp(const T& n, const T& a, const T& b) {
	return std::max(a, std::min(n, b));
}


class LPF {
public:
	void init() {
		_pos = 0;
		_speed = 0;
	}

	void set(float freq, float reso) {
		float w = 2 * M_PI * freq / MIXRATE;
		float q = 1 - w / (2 * (reso + 0.5 / (1 + w)) + w - 2);
		_r = q * q;
		_c = _r + 1 - 2 * cosf(w) * q;
	}
	float mix(float a) {
		_speed += (a - _pos) * _c;
		_pos += _speed;
		_speed *= _r;
		return _pos;
	}

private:
	float _r;
	float _c;

	float _pos;
	float _speed;
};


class Channel {
public:
	Channel() { init(); }

	void init();
	bool set_param_env(std::string name, Envelope env);
	void note_event(int note);
	void tick();
	void add_mix(float frame[2], const Channel& modulator);

	float get_level() const { return _level * _volume; }

private:
	enum class State { OFF, RELEASE, ATTACK, HOLD };
	enum class Wave { PULSE, TRIANGLE, SINE, NOISE, C64NOISE };

	State			_state;
	float			_level;
	float			_phase;
	float			_speed;
	unsigned int	_shift;


	float			_note;
	float			_offset;

	Wave			_wave;
	float			_pulsewidth;
	float			_volume;
	float			_panning[2];

	float			_resolution;
	float			_vibrato_phase;
	float			_vibrato_speed;
	float			_vibrato_depth;

	float			_attack;
	float			_decay;
	float			_sustain;
	float			_release;

	float			_amp;
	float			_ringmod;
	float			_cutoff;
	float			_resonance;
	LPF				_filter;


	enum class ParamID {
		WAVE,
		OFFSET,
		VOLUME,
		PANNING,
		PULSEWIDTH,
		RESOLUTION,
		VIBRATO_SPEED,
		VIBRATO_DEPTH,

		ATTACK,
		DECAY,
		SUSTAIN,
		RELEASE,

		RINGMOD,
		FILTER,
		CUTOFF,
		RESONANCE,

		PARAM_COUNT
	};

	std::array<Param,(int)ParamID::PARAM_COUNT> _params;
	void param_change(ParamID id, float v);
};

