#include <SDL/SDL.h>
#include "server.h"


void Server::init(Tune* tune) {

	// open log file
	SF_INFO info = { 0, MIXRATE, 2, SF_FORMAT_WAV | SF_FORMAT_PCM_16 };
	_log = sf_open("log.wav", SFM_WRITE, &info);


	// find midi device
	Pm_Initialize();
	_midi = nullptr;
	int dev_count = Pm_CountDevices();
	int i;
	for (i = 0; i < dev_count; i++) {
		const PmDeviceInfo* info = Pm_GetDeviceInfo(i);
		if (info->output &&
			std::string(info->name).find("MIDI") != std::string::npos) break;
	}
	if (i < dev_count) {
		Pm_OpenInput(&_midi, 3, nullptr, 0, nullptr, nullptr);
	}

	// start sound server
	static SDL_AudioSpec spec = { MIXRATE, AUDIO_S16SYS,
		2, 0, 1024, 0, 0, &Server::audio_callback, this
	};
	SDL_OpenAudio(&spec, &spec);

	_playing = false;
	_blockloop = false;
	_frame = 0;
	_tick = 0;
	_row = 0;
	_block = 0;
	_tune = tune;

	_ticks_per_row.init(_tune->ticks_per_row);

	SDL_PauseAudio(0);
}

Server::~Server() {
	SDL_CloseAudio();
	if (_log) sf_close(_log);
	if (_midi) Pm_Close(_midi);
	Pm_Terminate();
}


void Server::tick() {

//	if (_midi && i == _midi_channel_nr) {
//		struct { unsigned char type, val, x, y; } event;
//		for (;;) {
//			int l = Pm_Read(_midi, (PmEvent*) &event, 1);
//			if (!l) break;
//
//			static int last_note = 0;
//			string row;
//			if (event.type == 128 && event.val == last_note) row = "---";
//			else if (event.type == 144) {
//				int i = event.val;
//				row	= string(1, "ccddeffggaab"[i%12])
//					+ string(1, "-#-#--#-#-#-"[i%12])
//					+ string(1, '0' + i/12);
//				last_note = event.val;
//			}
//			if (!row.empty()) _channels[i].set_row_commands({ { "note", row } });
//		}
//	}

	// tick server
	if (_tick == 0) { // new row
		_ticks_per_row.tick();
	}

	// tick channels
	auto& line = _tune->table[_block];
	int i = 0;
	for (auto& chan : _channels) {
		if (_tick == 0) {
			auto& pat_name = line[i++];
			if (_tune->patterns.count(pat_name)) {
				auto& pat = _tune->patterns[pat_name];
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

bool Server::apply_macro(const std::string& macro_name, Channel& chan) const {
	if (macro_name == "") return false;
	auto it = _tune->macros.find(macro_name);
	if (it == _tune->macros.end()) return false;
	auto& macro = it->second;
	int res = true;
	for (auto& m : macro.parents) res &= apply_macro(m, chan);
	for (auto& p : macro.envs) res &= chan.set_param_env(p.first, p.second);
	return res;
}

void Server::mix(short* buffer, int length) {
	for (int i = 0; i < length; i += 2) {

		if (_playing) {
			if (_frame == 0) tick();
			if (++_frame >= _tune->frames_per_tick) {
				_frame = 0;
				if (++_tick >= _ticks_per_row.val()) {
					_tick = 0;
					if (++_row >= get_max_rows(_tune->table[_block], _tune->patterns)) {
						_row = 0;
						if (!_blockloop && ++_block >= (int) _tune->table.size()) _block = 0;
					}
				}
			}
		}
		float frame[2] = { 0, 0 };
		for (auto& chan : _channels) chan.add_mix(frame);
		buffer[i + 0] = clamp((int) (frame[0] * 6000), -32768, 32768);
		buffer[i + 1] = clamp((int) (frame[1] * 6000), -32768, 32768);
	}
	sf_writef_short(_log, buffer, length / 2);
}

