#pragma once

#include "param.h"


template <typename T>
T clamp(const T& n, const T& a, const T& b) {
	return std::max(a, std::min(n, b));
}



class Channel {
public:
	Channel() { init(); }

	void init();
	bool set_param_env(std::string name, Envelope env);
	void note_event(int note);
	void tick();
	void add_mix(float frame[2]);

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

		PARAM_COUNT
	};

	std::array<Param,(int)ParamID::PARAM_COUNT> _params;
	void param_change(ParamID id, float v);
};

