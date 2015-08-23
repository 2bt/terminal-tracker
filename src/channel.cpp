#include "channel.h"


void Channel::init() {
	_filter.init();
	_state = State::OFF;
	_shift = 0x7ffff8;
}

void Channel::note_event(int note) {
	if (note == -1) {
		_state = State::RELEASE;
	}
	if (note > 0) {
		_note = note;
		_state = State::ATTACK;
		_level = 0;
		_phase = 0;
		_vibrato_phase = 0;
	}
}

bool Channel::set_param_env(std::string name, Envelope env) {
	static const std::map<std::string,ParamID> m {
		{ "wave",			ParamID::WAVE			},
		{ "offset",			ParamID::OFFSET			},
		{ "volume",			ParamID::VOLUME			},
		{ "panning",		ParamID::PANNING		},
		{ "pulsewidth",		ParamID::PULSEWIDTH		},
		{ "resolution",		ParamID::RESOLUTION		},
		{ "vibratospeed",	ParamID::VIBRATO_SPEED	},
		{ "vibratodepth",	ParamID::VIBRATO_DEPTH	},
		{ "attack",			ParamID::ATTACK			},
		{ "decay",			ParamID::DECAY			},
		{ "sustain",		ParamID::SUSTAIN		},
		{ "release",		ParamID::RELEASE		},
		{ "cutoff",			ParamID::CUTOFF			},
		{ "resonance",		ParamID::RESONANCE		},
	};
	auto it = m.find(name);
	if (it == m.end()) return false;
	_params[(int) it->second].init(env);
	return true;
}


void Channel::param_change(ParamID id, float v) {
	switch (id) {

	case ParamID::OFFSET:		_offset		= v; break;
	case ParamID::WAVE:			_wave		= (Wave) v; break;
	case ParamID::PULSEWIDTH:	_pulsewidth	= fmodf(v, 1); break;
	case ParamID::VOLUME:		_volume		= std::max(0.0f, v); break;

	case ParamID::PANNING:
		_panning[0] = sqrtf(0.5 - clamp(v, -1.0f, 1.0f) * 0.5);
		_panning[1] = sqrtf(0.5 + clamp(v, -1.0f, 1.0f) * 0.5);
		break;

	case ParamID::RESOLUTION:		_resolution		= std::max(0.0f, v); break;
	case ParamID::VIBRATO_SPEED:	_vibrato_speed	= v; break;
	case ParamID::VIBRATO_DEPTH:	_vibrato_depth	= v; break;

	case ParamID::ATTACK:			_attack			= v; break;
	case ParamID::DECAY:			_decay			= v; break;
	case ParamID::SUSTAIN:			_sustain		= v; break;
	case ParamID::RELEASE:			_release		= v; break;

	case ParamID::RESONANCE:
		_resonance = v;
		if (v > 0) _filter.set(_cutoff, _resonance);
		break;
	case ParamID::CUTOFF:
		_cutoff = std::max<float>(10, v);
		_filter.set(_cutoff, _resonance);
		break;

	default: break;
	}

}

void Channel::tick() {
	int i = 0;
	for (auto& p : _params) {
		if (p.tick()) param_change((ParamID) i, p.val());
		i++;
	}

	// vibrato
	_vibrato_phase = fmodf(_vibrato_phase + _vibrato_speed, 1);
	float vib = sinf(_vibrato_phase * 2 * M_PI) * _vibrato_depth;
	_speed = powf(2, (_note + _offset + vib - 58) * (1 / 12.0)) * (440.0 / MIXRATE);

}

void Channel::add_mix(float frame[2]) {

	switch (_state) {
	case State::OFF: return;
	case State::ATTACK:
		_level += _attack;
		if (_level > 1) {
			_level = 1;
			_state = State::HOLD;
		}
		break;
	case State::HOLD:
		_level = _sustain + (_level - _sustain) * _decay;
		break;
	case State::RELEASE:
	default:
		_level *= _release;
		break;
	}


	_phase += _speed;
	if (_wave != Wave::C64NOISE) _phase = fmodf(_phase, 1);

	float amp = 0;

	switch (_wave) {
	case Wave::PULSE:
		amp = _phase < _pulsewidth ? -1 : 1;
		break;
	case Wave::TRIANGLE:
		amp = _phase < _pulsewidth ?
			2 / _pulsewidth * _phase - 1 :
			2 / (_pulsewidth - 1) * (_phase - _pulsewidth) + 1;
		break;
	case Wave::SINE:
		amp = sinf(_phase * 2 * M_PI);
		break;
	case Wave::NOISE:
		amp = rand() / float(RAND_MAX) * 2 - 1;
		break;
	case Wave::C64NOISE: {
		unsigned int s = _shift;
		unsigned int b;
		while (_phase > 0.1) {
			_phase -= 0.1;
			b = ((s >> 22) ^ (s >> 17)) & 1;
			s = ((s << 1) & 0x7fffff) + b;
		}
		_shift = s;
		amp = (
			((s & 0x400000) >> 11) |
			((s & 0x100000) >> 10) |
			((s & 0x010000) >> 7) |
			((s & 0x002000) >> 5) |
			((s & 0x000800) >> 4) |
			((s & 0x000080) >> 1) |
			((s & 0x000010) << 1) |
			((s & 0x000004) << 2)) * (2.0 / (1 << 12)) - 1;
		break;
	}
	default: break;
	}

	if (_resolution > 0) amp = floorf(amp * _resolution) / _resolution;
	if (_resonance > 0) amp = _filter.mix(amp);

	amp *= _level;
	amp *= _volume;

	frame[0] += amp * _panning[0];
	frame[1] += amp * _panning[1];
}

