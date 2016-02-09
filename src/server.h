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
	void	generate_full_log();
	void	start();

	void	play(int block=0, bool looping=false);
	void	pause();
	bool	is_playing() const { return _playing; }
	int		get_row()	const { return _row; }
	int		get_block()	const { return _block; }

	void	set_muted(int chan_nr, bool m) { _muted[chan_nr] = m; }
	bool	get_muted(int chan_nr) const { return _muted[chan_nr]; }

	float	get_chan_level(int chan_nr) const { return _channels[chan_nr].get_level(); }

	void	get_nearest_row(int& block_nr, int& row_nr) const;
	void	play_row(int chan_nr, const Row& row);

private:
	static void audio_callback(void* userdata, unsigned char* stream, int len) {
		((Server*) userdata)->mix((short*) stream, len / 2);
	}
	void mix(short* buffer, int length);
	void tick();
	bool apply_macro(const Macro& macro, Channel& chan) const;
	bool apply_macro(const std::string& macro_name, Channel& chan) const;

	SNDFILE*			_log;
	PortMidiStream*		_midi = nullptr;
	MidiCallback		_midi_callback = nullptr;
	std::mutex			_mutex;

	volatile bool 	_playing;
	volatile bool	_blockloop;
	volatile int	_frame;
	volatile int	_tick;
	volatile int	_row;
	volatile int	_block;
	Param			_ticks_per_row;

	SimpleParamBatch		_param_batch;


	std::array<bool,CHANNEL_COUNT>		_muted;
	std::array<Channel,CHANNEL_COUNT>	_channels;
	FX									_fx;

	Tune* _tune;
};

extern Server server;
