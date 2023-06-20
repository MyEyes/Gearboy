
// Gb_Snd_Emu 0.1.4. http://www.slack.net/~ant/

#include "Sound_Queue.h"

#include <assert.h>
#include <string.h>
#include <string>

/* Copyright (C) 2005 by Shay Green. Permission is hereby granted, free of
charge, to any person obtaining a copy of this software module and associated
documentation files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and
to permit persons to whom the Software is furnished to do so, subject to the
following conditions: The above copyright notice and this permission notice
shall be included in all copies or substantial portions of the Software. THE
SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

Sound_Queue::Sound_Queue()
{
	bufs = NULL;
	sound_open = false;
	sync_output = true;
}

Sound_Queue::~Sound_Queue()
{
	stop();
}

const char* Sound_Queue::start( long sample_rate, int chan_count )
{
	assert( !bufs ); // can only be initialized once
	(void)sample_rate;
	(void)chan_count;

	write_buf = 0;
	write_pos = 0;
	read_buf = 0;

	bufs = new sample_t [(long) buf_size * buf_count];
	if ( !bufs )
		return "Out of memory";
	currently_playing_ = bufs;

	for (long l = 0; l < ((long) buf_size * buf_count); l++)
		bufs[0] = 0;

	return NULL;
}

void Sound_Queue::stop()
{
	delete [] bufs;
	bufs = NULL;
}

int Sound_Queue::sample_count() const
{
	return buf_size * buf_count;
}

inline Sound_Queue::sample_t* Sound_Queue::buf( int index )
{
	assert( (unsigned) index < buf_count );
	return bufs + (long) index * buf_size;
}

void Sound_Queue::write( const sample_t* in, int count, bool sync )
{
	(void)in;
	(void)count;
	(void)sync;
}

void Sound_Queue::fill_buffer( uint8_t* out, int count )
{
	memset( out, 0, count );
}

void Sound_Queue::fill_buffer_( void* user_data, uint8_t* out, int count )
{
	(void)user_data;
	(void)out;
	(void)count;
}
