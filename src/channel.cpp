#include "channel.h"


void Channel::init() {
	_state = State::OFF;
	_level = 0;
	_filter_l = 0;
	_filter_b = 0;
	_filter_h = 0;
	_smooth[0] = 0;
	_smooth[1] = 0;

	static const EnvelopeMap default_envs = {
		{ "wave",			0		},
		{ "offset",			0		},
		{ "volume",			1		},
		{ "panning",		0		},
		{ "pulsewidth",		0.5		},
		{ "resolution",		0		},
		{ "vibratospeed",	0		},
		{ "vibratodepth",	0		},
		{ "attack",			0.01	},
		{ "decay",			0.5		},
		{ "sustain",		0.5		},
		{ "release",		0.2		},
		{ "sync",			0		},
		{ "ringmod",		0		},
		{ "filter",			0		},
		{ "resonance",		15		},
		{ "cutoff",			20		},
		{ "gliss",			0		},
		{ "pulsewidthsweep",0		},
		{ "echo",			0		},
	};
	set_param_envs(default_envs);

}

bool Channel::set_param_envs(const EnvelopeMap& envs) {
	bool res = true;
	for (auto& p : envs) res &= set_param_env(p.first, p.second);
	return res;
}

bool Channel::set_param_env(std::string name, const Envelope& env) {
	static const std::map<std::string,ParamID> m {
		{ "wave",			ParamID::WAVE			},
		{ "offset",			ParamID::OFFSET			},
		{ "volume",			ParamID::VOLUME			},
		{ "panning",		ParamID::PANNING		},
		{ "pulsewidth",		ParamID::PULSEWIDTH		},
		{ "pulsewidthsweep",ParamID::PULSEWIDTH_SWEEP},
		{ "resolution",		ParamID::RESOLUTION		},
		{ "vibratospeed",	ParamID::VIBRATO_SPEED	},
		{ "vibratodepth",	ParamID::VIBRATO_DEPTH	},
		{ "attack",			ParamID::ATTACK			},
		{ "decay",			ParamID::DECAY			},
		{ "sustain",		ParamID::SUSTAIN		},
		{ "release",		ParamID::RELEASE		},
		{ "sync",			ParamID::SYNC			},
		{ "ringmod",		ParamID::RINGMOD		},
		{ "filter",			ParamID::FILTER			},
		{ "cutoff",			ParamID::CUTOFF			},
		{ "resonance",		ParamID::RESONANCE		},
		{ "gliss",			ParamID::GLISS			},
		{ "echo",			ParamID::ECHO			},
	};
	auto it = m.find(name);
	if (it == m.end()) return false;
	_params[(int) it->second].init(env);
	return true;
}

void Channel::param_change(ParamID id, float v) {
	switch (id) {

	case ParamID::OFFSET:			_offset			= v; break;
	case ParamID::WAVE:				_wave			= (Wave) v; break;
	case ParamID::PULSEWIDTH:		_pulsewidth		= fmodf(v, 1); break;
	case ParamID::VOLUME:			_volume			= clamp<float>(v, 0, 5); break;
	case ParamID::PULSEWIDTH_SWEEP:	_pulsewidth_sweep = v / 1000; break;
	case ParamID::GLISS:			_gliss			= std::max(0.0f, v); break;

	case ParamID::PANNING:
		_panning[0] = sqrtf(0.5 - clamp<float>(v, -1, 1) * 0.5);
		_panning[1] = sqrtf(0.5 + clamp<float>(v, -1, 1) * 0.5);
		break;

	case ParamID::RESOLUTION:		_resolution		= std::max(0.0f, v); break;
	case ParamID::VIBRATO_SPEED:	_vibrato_speed	= v; break;
	case ParamID::VIBRATO_DEPTH:	_vibrato_depth	= v; break;

	case ParamID::ATTACK:			_attack			= 1.0 / 44100 / clamp(v); break;
	case ParamID::DECAY:			_decay			= exp(log(0.01) / 44100 / v); break;
	case ParamID::SUSTAIN:			_sustain		= clamp(v); break;
	case ParamID::RELEASE:			_release		= exp(log(0.01) / 44100 / v); break;

	case ParamID::SYNC:				_sync			= v > 0; break;
	case ParamID::RINGMOD:			_ringmod		= clamp(v); break;


	case ParamID::FILTER:			_filter			= (uint32_t) v; break;
	case ParamID::RESONANCE:		_resonance		= 1.1 - 0.04 * clamp<float>(v, 0, 15); break;
	case ParamID::CUTOFF: 			_cutoff			= clamp<float>(v, 0, 100) * 0.01; break;

	case ParamID::ECHO: 			_echo			= clamp<float>(v, 0, 1); break;

	default: break;
	}
}

void Channel::note_event(int note) {
	if (note == -1) {
		_state = State::RELEASE;
	}
	if (note > 0) {
		_dst_note = note;
		_state = State::ATTACK;
		_level = 0;
		_phase = 0;
		_vibrato_phase = 0;
	}
}

void Channel::tick() {
	int i = 0;
	for (auto& p : _params) {
		if (p.tick()) param_change((ParamID) i, p.val());
		i++;
	}



	_pulsewidth = fmodf(_pulsewidth + _pulsewidth_sweep, 1);


	if (_gliss > 0) {
		if (_note < _dst_note) {
			_note = std::min(_note + _gliss, _dst_note);
		}
		else {
			_note = std::max(_note - _gliss, _dst_note);
		}
	}
	else _note = _dst_note;


	// vibrato
	_vibrato_phase = fmodf(_vibrato_phase + _vibrato_speed, 1);
	float vib = sinf(_vibrato_phase * 2 * M_PI) * _vibrato_depth;
	_speed = powf(2, (_note + _offset + vib - 58) * (1 / 12.0)) * (440.0 / MIXRATE);

}

void Channel::add_mix(float* frame, const Channel& modulator, FX& fx) {

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

	// sync
	if (_sync && modulator._phase < modulator._speed) {
		_phase = modulator._phase / modulator._speed * _speed;
	}

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

	// low rez
	if (_resolution > 0) amp = floorf(amp * _resolution) / _resolution;


	// filter
	if (_filter) {

		_filter_h = amp - _filter_b * _resonance - _filter_l;
		_filter_b += _cutoff * _filter_h;
		_filter_l += _cutoff * _filter_b;

		amp = 0;

		if (_filter & 1) amp += _filter_l;
		if (_filter & 2) amp += _filter_b;
		if (_filter & 4) amp += _filter_h;
	}


	// ringmod
	_amp = amp;
	amp = amp * (1 - _ringmod) + modulator._amp * amp * _ringmod;


	// smooth volume change
	{
		float v;
		const float s = 0.97;
		v = _level * _volume * _panning[0]; _smooth[0] = _smooth[0] * s + v * (1 - s);
		v = _level * _volume * _panning[1]; _smooth[1] = _smooth[1] * s + v * (1 - s);
	}

	float out[] = { amp * _smooth[0], amp * _smooth[1] };
	frame[0] += out[0];
	frame[1] += out[1];

	// global effects
	fx.echo(out[0] * _echo, out[1] * _echo);
}

