#include <SDL/SDL.h>
#include "server.h"


void Server::init(Tune* tune, MidiCallback* callback) {
	_tune = tune;
	_playing = false;
	_blockloop = false;
	_block = 0;
	_frame = 0;
	_tick = 0;
	_row = 0;
	init_channels();

	_midi_callback = callback;

	// open log file
	SF_INFO info = { 0, MIXRATE, 2, SF_FORMAT_WAV | SF_FORMAT_PCM_16 };
	_log = sf_open("log.wav", SFM_WRITE, &info);
}

void Server::generate_full_log() {
	play();
	short buffer[_tune->frames_per_tick * 2];
	do {
		mix(buffer, _tune->frames_per_tick * 2);
	}
	while (!(_frame == 0 && _tick == 0 && _row == 0 && _block == 0));
	pause();
}


void Server::start() {
	// midi
	Pm_Initialize();
	int dev_count = Pm_CountDevices();
	int i;
	for (i = 0; i < dev_count; i++) {
		auto info = Pm_GetDeviceInfo(i);
		if (info->output &&
			std::string(info->name).find("MIDI") != std::string::npos) break;
	}
	if (i < dev_count) Pm_OpenInput(&_midi, 3, nullptr, 0, nullptr, nullptr);

	// start sound server
	static SDL_AudioSpec spec = { MIXRATE, AUDIO_S16SYS,
		2, 0, 1024, 0, 0, &Server::audio_callback, this
	};
	SDL_OpenAudio(&spec, &spec);
	SDL_PauseAudio(0);
}

Server::~Server() {
	SDL_CloseAudio();
	if (_log) sf_close(_log);
	if (_midi) Pm_Close(_midi);
	Pm_Terminate();
}

void Server::init_channels() {
	static const Macro default_macro = {
		{},
		{
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
		}
	};
	for (auto& chan : _channels) {
		chan.init();
		apply_macro(default_macro, chan);
	}
}

void Server::play(int block, bool looping) {
	std::lock_guard<std::mutex> guard(_mutex);
	_playing = true;
	_blockloop = looping;
	_block = block;
	_frame = 0;
	_tick = 0;
	_row = 0;
	init_channels();
}

void Server::pause() {
	std::lock_guard<std::mutex> guard(_mutex);
	_playing = false;
	for (auto& chan : _channels) chan.note_event(-1);
}

Row* Server::get_nearest_row(int chan_nr) {
	int row = _row;
	int block = _block;
	if (_tick >= _ticks_per_row.val() / 2) {
		if (++row >= get_max_rows(*_tune, _block)) {
			row = 0;
			if (!_blockloop && ++block >= (int) _tune->table.size()) block = 0;
		}
	}
	auto& pn = _tune->table[block][chan_nr];
	auto it = _tune->patterns.find(pn);
	if (it == _tune->patterns.end()) return nullptr;
	auto& pat = it->second;
	if (row >= (int) pat.size()) return nullptr;
	return &pat[row];
}

void Server::tick() {

	// rough and ready midi support
	if (_midi) {
		struct { unsigned char type, val, x, y; } event;
		for (;;) {
			if (!Pm_Read(_midi, (PmEvent*) &event, 1)) break;
			_midi_callback(event.type, event.val);
		}
	}


	// tick server
	if (_playing && _tick == 0) { // new row
		if (_row == 0) _ticks_per_row.init(_tune->ticks_per_row);
		_ticks_per_row.tick();
	}

	// tick channels
	auto& line = _tune->table[_block];
	for (int i = 0; i < CHANNEL_COUNT; i++) {
		auto& chan = _channels[i];
		if (_playing && !_muted[i] && _tick == 0) {
			auto it = _tune->patterns.find(line[i]);
			if (it != _tune->patterns.end()) {
				auto& pat = it->second;
				if (_row < (int) pat.size()) {
					auto& row = pat[_row];
					if (row.note != 0) chan.note_event(row.note);
					for (auto& m : row.macros) apply_macro(m, chan);
				}
			}
		}
		chan.tick();
	}
}

bool Server::apply_macro(const Macro& macro, Channel& chan) const {
	int res = true;
	for (auto& m : macro.parents) res &= apply_macro(m, chan);
	for (auto& p : macro.envs) res &= chan.set_param_env(p.first, p.second);
	return res;
}

bool Server::apply_macro(const std::string& macro_name, Channel& chan) const {
	if (macro_name == "") return false;
	auto it = _tune->macros.find(macro_name);
	if (it == _tune->macros.end()) return false;
	return apply_macro(it->second, chan);
}

void Server::mix(short* buffer, int length) {
	std::lock_guard<std::mutex> guard(_mutex);
	for (int i = 0; i < length; i += 2) {

		if (_frame == 0) tick();
		if (++_frame >= _tune->frames_per_tick) {
			_frame = 0;
			if (_playing) {
				if (++_tick >= _ticks_per_row.val()) {
					_tick = 0;
					if (++_row >= get_max_rows(*_tune, _block)) {
						_row = 0;
						if (!_blockloop && ++_block >= (int) _tune->table.size()) _block = 0;
					}
				}
			}
		}
		float frame[2] = { 0, 0 };
		for (int i = 0; i < CHANNEL_COUNT; i++) {
			_channels[i].add_mix(frame, _channels[(i + CHANNEL_COUNT - 1) % CHANNEL_COUNT]);
		}
		buffer[i + 0] = clamp((int) (frame[0] * 6000), -32768, 32767);
		buffer[i + 1] = clamp((int) (frame[1] * 6000), -32768, 32767);
	}
	sf_writef_short(_log, buffer, length / 2);
}

