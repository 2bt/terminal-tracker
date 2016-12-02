#pragma once

#include <mutex>
#include <functional>
#include <sndfile.h>
#include <portmidi.h>

#include "channel.h"
#include "fx.h"



class Server {
public:
	using MidiCallback = std::function<void(int,int)>;
	~Server();
	void	init(Tune* tune, MidiCallback callback);
	void	generate_full_log(int subtune, int reps);
	void	start();

	void	play(int line=0, int row=0, bool looping=false);
	void	pause();
	bool	is_playing() const { return m_playing; }
	int		get_row()	const { return m_row; }
	int		get_line()	const { return m_line; }

	void	set_muted(int chan_nr, bool m) { m_muted[chan_nr] = m; }
	bool	get_muted(int chan_nr) const { return m_muted[chan_nr]; }

	float	get_chan_level(int chan_nr) const { return m_channels[chan_nr].get_level(); }

	void	get_nearest_row(int& line_nr, int& row_nr) const;
	void	play_row(int chan_nr, const Row& row);

private:
	static void audio_callback(void* userdata, unsigned char* stream, int len) {
		((Server*) userdata)->mix((short*) stream, len / 2);
	}
	void mix(short* buffer, int length);
	void tick();
	void apply_macro(const Macro& macro, Channel& chan) const;
	void apply_macro(const std::string& macro_name, Channel& chan) const;

	SNDFILE*			m_log;
	PortMidiStream*		m_midi = nullptr;
	MidiCallback		m_midi_callback = nullptr;
	std::mutex			m_mutex;

	volatile bool 	m_playing;
	volatile bool	m_lineloop;
	volatile int	m_frame;
	volatile int	m_tick;
	volatile int	m_row;
	volatile int	m_line;
	Param			m_ticks_per_row;

	SimpleParamBatch		m_param_batch;


	std::array<bool,CHANNEL_COUNT>		m_muted;
	std::array<Channel,CHANNEL_COUNT>	m_channels;
	FX									m_fx;

	Tune* m_tune;
};

extern Server server;
