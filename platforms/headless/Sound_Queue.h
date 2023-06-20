
// Simple headless sound queue that discards any audio

// Copyright (C) 2005 Shay Green. MIT license.

#ifndef SOUND_QUEUE_H
#define SOUND_QUEUE_H
#include <stdint.h>

// Simple SDL sound wrapper that has a synchronous interface
class Sound_Queue {
public:
	Sound_Queue();
	~Sound_Queue();

	// Initialize with specified sample rate and channel count.
	// Returns NULL on success, otherwise error string.
	const char* start( long sample_rate, int chan_count = 1 );

	// Number of samples in buffer waiting to be played
	int sample_count() const;

	// Write samples to buffer and block until enough space is available
	typedef short sample_t;
	void write( const sample_t*, int count, bool sync = true );

	// Pointer to samples currently playing (for showing waveform display)
	sample_t const* currently_playing() const { return currently_playing_; }

	// Stop audio output
	void stop();

private:
	enum { buf_size = 2048 };
	enum { buf_count = 3 };
	sample_t* volatile bufs;
	sample_t* volatile currently_playing_;
	int volatile read_buf;
	int write_buf;
	int write_pos;
	bool sound_open;
	bool sync_output;

	sample_t* buf( int index );
	void fill_buffer( uint8_t*, int );
	static void fill_buffer_( void*, uint8_t*, int );
};

#endif
