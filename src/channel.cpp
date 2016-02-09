#include "channel.h"


Channel::Channel() {
	init();
}

void Channel::init() {
	_state = State::OFF;
	_level = 0;
	_filter_l = 0;
	_filter_b = 0;
	_filter_h = 0;
	_smooth[0] = 0;
	_smooth[1] = 0;
	reset_params();
}

const std::map<std::string,int> Channel::_param_mapping = {
	{ "wave",			PID_WAVE			},
	{ "offset",			PID_OFFSET			},
	{ "volume",			PID_VOLUME			},
	{ "panning",		PID_PANNING			},
	{ "pulsewidth",		PID_PULSEWIDTH		},
	{ "pulsewidthsweep",PID_PULSEWIDTH_SWEEP},
	{ "resolution",		PID_RESOLUTION		},
	{ "vibratospeed",	PID_VIBRATO_SPEED	},
	{ "vibratodepth",	PID_VIBRATO_DEPTH	},
	{ "attack",			PID_ATTACK			},
	{ "decay",			PID_DECAY			},
	{ "sustain",		PID_SUSTAIN			},
	{ "release",		PID_RELEASE			},
	{ "sync",			PID_SYNC			},
	{ "ringmod",		PID_RINGMOD			},
	{ "filter",			PID_FILTER			},
	{ "cutoff",			PID_CUTOFF			},
	{ "resonance",		PID_RESONANCE		},
	{ "gliss",			PID_GLISS			},
	{ "echo",			PID_ECHO			},
};


void Channel::reset_params() {
	static const EnvelopeMap envs = {
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
	configure_params(envs);
}

bool Channel::configure_params(const EnvelopeMap& envs) {
	return _param_batch.configure(envs);
}

void Channel::note_event(int note) {
	if (note == -1) {
		if (_state != State::OFF) _state = State::RELEASE;
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
	_param_batch.tick([this](int pid, float v) {
		switch (pid) {

		case PID_OFFSET:			_offset			= v; break;
		case PID_WAVE:				_wave			= (Wave) v; break;
		case PID_PULSEWIDTH:		_pulsewidth		= fmodf(v, 1); break;
		case PID_VOLUME:			_volume			= clamp<float>(v, 0, 5); break;
		case PID_PULSEWIDTH_SWEEP:	_pulsewidth_sweep = v / 1000; break;
		case PID_GLISS:				_gliss			= std::max(0.0f, v); break;

		case PID_PANNING:
			_panning[0] = sqrtf(0.5 - clamp<float>(v, -1, 1) * 0.5);
			_panning[1] = sqrtf(0.5 + clamp<float>(v, -1, 1) * 0.5);
			break;

		case PID_RESOLUTION:		_resolution		= std::max(0.0f, v); break;
		case PID_VIBRATO_SPEED:	_vibrato_speed	= v; break;
		case PID_VIBRATO_DEPTH:	_vibrato_depth	= v; break;

		case PID_ATTACK:			_attack			= 1.0 / 44100 / clamp(v); break;
		case PID_DECAY:				_decay			= expf(log(0.01) / 44100 / v); break;
		case PID_SUSTAIN:			_sustain		= clamp(v); break;
		case PID_RELEASE:			_release		= expf(log(0.01) / 44100 / v); break;

		case PID_SYNC:				_sync			= v > 0; break;
		case PID_RINGMOD:			_ringmod		= clamp(v); break;


		case PID_FILTER:			_filter			= (uint32_t) v; break;
		case PID_RESONANCE:			_resonance		= 1.1 - 0.04 * clamp<float>(v, 0, 15); break;
		case PID_CUTOFF: 			_cutoff			= clamp<float>(v, 0, 100) * 0.01; break;

		case PID_ECHO: 				_echo			= clamp<float>(v, 0, 1); break;

		default: break;
		}
	});



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
		if (_level < 0.0001) _state = State::OFF;
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

