#pragma once

#include "tune.h"


template <typename T>
T clamp(const T& n, const T& a, const T& b) {
	return std::max(a, std::min(n, b));
}



class Channel {
public:
	Channel() { init(); }

	void init();
	void set_param_env(std::string name, Envelope env);
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


	class Param {
	public:
		void init(Envelope env) {
			_env = env;
			_pos = 0;
			_value = 0;
			_first_tick = true;
		}
		bool tick() {
			if (_pos >= (int) _env.nodes.size()) {
				if (_env.loop == -1) return false;
				_pos = _env.loop;
			}
			float v = _value;
			auto& n = _env.nodes[_pos];
			if (n.delta) _value += n.value;
			else _value = n.value;
			_pos++;
			if (_first_tick) {
				_first_tick = false;
				return true;
			}
			return _value != v;
		}
		float val() const { return _value; }
	private:
		Envelope	_env;
		int			_pos = 0;
		float		_value = 0;
		bool		_first_tick = true;
	};

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

		PARAM_COUNT
	};

	std::array<Param,(int)ParamID::PARAM_COUNT> _params;
	void param_change(ParamID id, float v);
};

