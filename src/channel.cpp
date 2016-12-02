#include <stdlib.h>
#include "channel.h"


Channel::Channel() {
	init();
}

void Channel::init() {
	m_state = State::OFF;
	m_level = 0;
	m_filter_l = 0;
	m_filter_b = 0;
	m_filter_h = 0;
	m_smooth[0] = 0;
	m_smooth[1] = 0;
	reset_params();
}

const std::map<std::string,int> Channel::m_param_mapping = {
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

void Channel::configure_params(const EnvelopeMap& envs) {
	m_param_batch.configure(envs);
}

void Channel::note_event(int note) {
	if (note == -1) {
		if (m_state != State::OFF) m_state = State::RELEASE;
	}
	if (note > 0) {
		m_dst_note = note;
		m_state = State::ATTACK;
		m_level = 0;
		m_phase = 0;
		m_vibrato_phase = 0;
	}
}


void Channel::tick() {

	m_param_batch.tick([this](int pid, float v) {
		switch (pid) {

		case PID_OFFSET:			m_offset			= v; break;
		case PID_WAVE:				m_wave			= (Wave) v; break;
		case PID_PULSEWIDTH:		m_pulsewidth		= fmodf(v, 1); break;
		case PID_VOLUME:			m_volume			= clamp<float>(v, 0, 5); break;
		case PID_PULSEWIDTH_SWEEP:	m_pulsewidth_sweep = v / 1000; break;
		case PID_GLISS:				m_gliss			= std::max(0.0f, v); break;

		case PID_PANNING:
			m_panning[0] = sqrtf(0.5 - clamp<float>(v, -1, 1) * 0.5);
			m_panning[1] = sqrtf(0.5 + clamp<float>(v, -1, 1) * 0.5);
			break;

		case PID_RESOLUTION:		m_resolution		= std::max(0.0f, v); break;
		case PID_VIBRATO_SPEED:	m_vibrato_speed	= v; break;
		case PID_VIBRATO_DEPTH:	m_vibrato_depth	= v; break;

		case PID_ATTACK:			m_attack			= 1.0 / 44100 / clamp(v); break;
		case PID_DECAY:				m_decay			= expf(log(0.01) / 44100 / v); break;
		case PID_SUSTAIN:			m_sustain		= clamp(v); break;
		case PID_RELEASE:			m_release		= expf(log(0.01) / 44100 / v); break;

		case PID_SYNC:				m_sync			= v > 0; break;
		case PID_RINGMOD:			m_ringmod		= clamp(v); break;


		case PID_FILTER:			m_filter			= (uint32_t) v; break;
		case PID_RESONANCE:			m_resonance		= 1.1 - 0.04 * clamp<float>(v, 0, 15); break;
		case PID_CUTOFF: 			m_cutoff			= clamp<float>(v, 0, 100) * 0.01; break;

		case PID_ECHO: 				m_echo			= clamp<float>(v, 0, 1); break;

		default: break;
		}
	});


	m_pulsewidth = fmodf(m_pulsewidth + m_pulsewidth_sweep, 1);


	if (m_gliss > 0) {
		if (m_note < m_dst_note) {
			m_note = std::min(m_note + m_gliss, m_dst_note);
		}
		else {
			m_note = std::max(m_note - m_gliss, m_dst_note);
		}
	}
	else m_note = m_dst_note;


	// vibrato
	m_vibrato_phase = fmodf(m_vibrato_phase + m_vibrato_speed, 1);
	float vib = sinf(m_vibrato_phase * 2 * M_PI) * m_vibrato_depth;
	m_speed = powf(2, (m_note + m_offset + vib - 58) * (1 / 12.0)) * (440.0 / MIXRATE);

}

void Channel::add_mix(float* frame, const Channel& modulator, FX& fx) {

	switch (m_state) {
	case State::OFF: return;
	case State::ATTACK:
		m_level += m_attack;
		if (m_level > 1) {
			m_level = 1;
			m_state = State::HOLD;
		}
		break;
	case State::HOLD:
		m_level = m_sustain + (m_level - m_sustain) * m_decay;
		break;
	case State::RELEASE:
	default:
		m_level *= m_release;
		if (m_level < 0.0001) m_state = State::OFF;
		break;
	}


	m_phase += m_speed;
	if (m_wave != Wave::C64NOISE) m_phase = fmodf(m_phase, 1);

	// sync
	if (m_sync && modulator.m_phase < modulator.m_speed) {
		m_phase = modulator.m_phase / modulator.m_speed * m_speed;
	}

	float amp = 0;

	switch (m_wave) {
	case Wave::PULSE:
		amp = m_phase < m_pulsewidth ? -1 : 1;
		break;
	case Wave::TRIANGLE:
		amp = m_phase < m_pulsewidth ?
			2 / m_pulsewidth * m_phase - 1 :
			2 / (m_pulsewidth - 1) * (m_phase - m_pulsewidth) + 1;
		break;
	case Wave::SINE:
		amp = sinf(m_phase * 2 * M_PI);
		break;
	case Wave::NOISE:
		amp = rand() / float(RAND_MAX) * 2 - 1;
		break;
	case Wave::C64NOISE: {
		unsigned int s = m_shift;
		unsigned int b;
		while (m_phase > 0.1) {
			m_phase -= 0.1;
			b = ((s >> 22) ^ (s >> 17)) & 1;
			s = ((s << 1) & 0x7fffff) + b;
		}
		m_shift = s;
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
	if (m_resolution > 0) amp = floorf(amp * m_resolution) / m_resolution;


	// filter
	if (m_filter) {

		m_filter_h = amp - m_filter_b * m_resonance - m_filter_l;
		m_filter_b += m_cutoff * m_filter_h;
		m_filter_l += m_cutoff * m_filter_b;

		amp = 0;

		if (m_filter & 1) amp += m_filter_l;
		if (m_filter & 2) amp += m_filter_b;
		if (m_filter & 4) amp += m_filter_h;
	}


	// ringmod
	m_amp = amp;
	amp = amp * (1 - m_ringmod) + modulator.m_amp * amp * m_ringmod;


	// smooth volume change
	{
		float v;
		const float s = 0.97;
		v = m_level * m_volume * m_panning[0]; m_smooth[0] = m_smooth[0] * s + v * (1 - s);
		v = m_level * m_volume * m_panning[1]; m_smooth[1] = m_smooth[1] * s + v * (1 - s);
	}

	float out[] = { amp * m_smooth[0], amp * m_smooth[1] };
	frame[0] += out[0];
	frame[1] += out[1];

	// global effects
	fx.echo(out[0] * m_echo, out[1] * m_echo);
}

