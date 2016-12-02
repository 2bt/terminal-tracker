#include <SDL/SDL.h>
#include "server.h"


void Server::init(Tune* tune, MidiCallback callback) {
	m_tune = tune;
	m_playing = false;
	m_lineloop = false;
	m_line = 0;
	m_frame = 0;
	m_tick = 0;
	m_row = 0;
	for (auto& chan : m_channels) chan.init();
	m_fx.init();

	m_midi_callback = callback;

	// open log file
	SF_INFO info = { 0, MIXRATE, 2, SF_FORMAT_WAV | SF_FORMAT_PCM_16 };
	m_log = sf_open("log.wav", SFM_WRITE, &info);
}

void Server::generate_full_log(int subtune, int reps) {
	int start_line = 0;
	while (subtune-- > 0) {
		auto& table = m_tune->table;
		while (start_line < table.size() - 1 && table[start_line] != TableLine()) start_line++;
		while (start_line < table.size() - 1 && table[start_line] == TableLine()) start_line++;
	}

	play(start_line);
	short buffer[m_tune->frames_per_tick * 2];
	while (reps-- > 0) {
		do 	mix(buffer, m_tune->frames_per_tick * 2);
		while (!(m_frame == 0 && m_tick == 0 && m_row == 0 && m_line == start_line));
	}
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
	if (i < dev_count) Pm_OpenInput(&m_midi, 3, nullptr, 0, nullptr, nullptr);

	// start sound server
	static SDL_AudioSpec spec = { MIXRATE, AUDIO_S16SYS,
		2, 0, 1024, 0, 0, &Server::audio_callback, this
	};
	SDL_OpenAudio(&spec, &spec);
	SDL_PauseAudio(0);
}

Server::~Server() {
	SDL_CloseAudio();
	if (m_log) sf_close(m_log);
	if (m_midi) Pm_Close(m_midi);
	Pm_Terminate();
}

void Server::play(int line, int row, bool looping) {
	std::lock_guard<std::mutex> guard(m_mutex);
	m_playing = true;
	m_lineloop = looping;
	m_line = line;
	m_row = row;
	m_frame = 0;
	m_tick = 0;
	for (auto& chan : m_channels) chan.init();
	m_fx.init();

	// pre-configure params
	m_ticks_per_row.init(m_tune->ticks_per_row);
	m_param_batch.configure(m_tune->envs);
}

void Server::pause() {
	std::lock_guard<std::mutex> guard(m_mutex);
	m_playing = false;
	for (auto& chan : m_channels) chan.note_event(-1);
}

void Server::get_nearest_row(int& line_nr, int& row_nr) const {
	row_nr = m_row;
	line_nr = m_line;
	if (m_tick >= m_ticks_per_row.val() / 2) {
		if (++row_nr >= get_max_rows(*m_tune, m_line)) {
			row_nr = 0;
			if (!m_lineloop && ++line_nr >= (int) m_tune->table.size()) line_nr = 0;
		}
	}
}

void Server::play_row(int chan_nr, const Row& row) {
	auto& chan = m_channels[chan_nr];
	for (auto& m : row.macros) apply_macro(m, chan);
	if (row.note != 0) chan.note_event(row.note);
}

void Server::tick() {

	// rough and ready midi support
	if (m_midi) {
		struct { unsigned char type, val, x, y; } event;
		for (;;) {
			if (!Pm_Read(m_midi, (PmEvent*) &event, 1)) break;
			m_midi_callback(event.type, event.val);
		}
	}


	// tick server
	if (m_playing && m_tick == 0) { // new row
		if (m_row == 0) {
			m_ticks_per_row.init(m_tune->ticks_per_row);
			m_param_batch.configure(m_tune->envs);
		}
		m_ticks_per_row.tick();
		m_param_batch.tick([this](const std::string name, float v) {
			if (name == "echo_length") {
				m_fx.m_echo_length = clamp<int>(m_tune->frames_per_tick * v, 100, FX::MAX_ECHO_LENGTH);
			}
			else if (name == "echo_feedback") {
				m_fx.m_echo_feedback = clamp<float>(v, 0, 0.99);
			}
		});
	}

	// tick channels
	auto& line = m_tune->table[m_line];
	for (int i = 0; i < CHANNEL_COUNT; i++) {
		auto& chan = m_channels[i];
		if (m_playing && !m_muted[i] && m_tick == 0) {
			auto it = m_tune->patterns.find(line[i]);
			if (it != m_tune->patterns.end()) {
				auto& pat = it->second;
				if (m_row < (int) pat.size()) {
					auto& row = pat[m_row];
					for (auto& m : row.macros) apply_macro(m, chan);
					if (row.note != 0) chan.note_event(row.note);
				}
			}
		}
		chan.tick();
	}
}

void Server::apply_macro(const Macro& macro, Channel& chan) const {
	for (auto& m : macro.parents) apply_macro(m, chan);
	chan.configure_params(macro.envs);
}

void Server::apply_macro(const std::string& macro_name, Channel& chan) const {
	if (macro_name == "default") {
		chan.reset_params();
		return;
	}
	if (macro_name == "") return;
	auto it = m_tune->macros.find(macro_name);
	if (it == m_tune->macros.end()) return;
	apply_macro(it->second, chan);
}


static short map_amp(float x) {
	return int(tanhf(x * 0.5) * 32767);
}

void Server::mix(short* buffer, int length) {
	std::lock_guard<std::mutex> guard(m_mutex);
	for (int i = 0; i < length; i += 2) {

		if (m_frame == 0) tick();
		if (++m_frame >= m_tune->frames_per_tick) {
			m_frame = 0;
			if (m_playing) {
				if (++m_tick >= m_ticks_per_row.val()) {
					m_tick = 0;
					if (++m_row >= get_max_rows(*m_tune, m_line)) {
						m_row = 0;
						if (!m_lineloop && (++m_line >= (int) m_tune->table.size()
						|| m_tune->table[m_line] == TableLine())) {
							if (m_tune->table.size() < 2) m_line = 0;
							else while (--m_line > 0) { // find empty table line
								if (m_tune->table[m_line - 1] == TableLine()) break;
							}

						}
					}
				}
			}
		}

		float frame[2] = { 0, 0 };
		for (int i = 0; i < CHANNEL_COUNT; i++) {
			m_channels[i].add_mix(frame, m_channels[(i + CHANNEL_COUNT - 1) % CHANNEL_COUNT], m_fx);
		}
		m_fx.add_mix(frame);


		buffer[i + 0] = clamp((int) (frame[0] * 8000), -32768, 32767);
		buffer[i + 1] = clamp((int) (frame[1] * 8000), -32768, 32767);
//		buffer[i + 0] = map_amp(frame[0]);
//		buffer[i + 1] = map_amp(frame[1]);
	}
	sf_writef_short(m_log, buffer, length / 2);
}

