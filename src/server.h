#pragma once

#include <sndfile.h>
#include <portmidi.h>

#include "channel.h"



class Server {
public:
	using MidiCallback = void(int note);
	~Server();
	void	init(Tune* tune, MidiCallback* callback);

	void	play(int block=0, bool looping=false);
	void	stop();
	bool	is_playing() const { return _playing; }
	int		get_row()	const { return _row; }
	int		get_block()	const { return _block; }

	float	get_chan_level(int chan_nr) const { return _channels[chan_nr].get_level(); }

	Row*	get_nearest_row(int chan_nr);
	void	play_row(int chan_nr, const Row& row) {
		auto& chan = _channels[chan_nr];
		if (row.note != 0) chan.note_event(row.note);
		for (auto& m : row.macros) apply_macro(m, chan);
	}

private:
	static void audio_callback(void* userdata, unsigned char* stream, int len) {
		((Server*) userdata)->mix((short*) stream, len / 2);
	}
	void mix(short* buffer, int length);
	void tick();
	bool apply_macro(const Macro& macro, Channel& chan) const;
	bool apply_macro(const std::string& macro_name, Channel& chan) const;
	void init_channels();

	SNDFILE*			_log;
	PortMidiStream*		_midi = nullptr;
	MidiCallback*		_midi_callback = nullptr;

	bool 	_playing;
	bool	_blockloop;
	int		_frame;
	int		_tick;
	int		_row;
	int		_block;
	Param	_ticks_per_row;

	std::array<Channel,CHANNEL_COUNT> _channels;
	Tune* _tune;
};

extern Server server;
