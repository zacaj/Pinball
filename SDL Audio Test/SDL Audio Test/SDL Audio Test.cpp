// SDL Audio Test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <SDL1/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

typedef struct
{
	void* bank;
	uint32_t pin;
} IOPin;
static Uint8 *audio_chunk;
static Uint32 audio_len;
static Uint8 *audio_pos;

/* The audio function callback takes the following parameters:
stream:  A pointer to the audio buffer to be filled
len:     The length (in bytes) of the audio buffer
*/
#define max(a,b) ((((a))>((b))?((a)):((b))))

uint32_t a = 0;
void setOut(IOPin pin,int i)
{}

const IOPin LATCH = { 0, 0 };
const IOPin CLOCK = { 0, 0 };
const IOPin DATA = { 0, 0 };
double tone = 500.00;
uint8_t s;
uint32_t audio(void *data)
{
	setOut(CLOCK, 0);
	setOut(LATCH, 0);
	s = (sin(((double)a)*2.0*3.14159 / 8000.0*tone) + 1) * max(sin(((double)a)*2.0*3.14159 / 8000.0),0)*127;
	uint16_t d = 28672 | (s << 4);
	for (int i = 15; i >= 0; i--)
	{
		setOut(CLOCK, 0);
		setOut(DATA, (d & 1 << i) ? 1 : 0);
		setOut(CLOCK, 1);
	}
	setOut(LATCH, 1);
	a++;
	return 0;
}
void fill_audio(void *udata, Uint8 *stream, int len)
{
	Uint8 *str = stream;
	for (int i = 0; i < len; i++)
	{
		audio(NULL);
		*str = s;
		str++;
	}
}



int _tmain(int argc, _TCHAR* argv[])
{
	SDL_Init(SDL_INIT_AUDIO);

	SDL_AudioSpec wanted;
	/* Set the audio format */
	wanted.freq = 8000;
	wanted.format = AUDIO_U8;
	wanted.channels = 1;    /* 1 = mono, 2 = stereo */
	wanted.samples = 1024;  /* Good low-latency value for callback */
	wanted.callback = fill_audio;
	wanted.userdata = NULL;

	/* Open the audio device, forcing the desired format */
	if (SDL_OpenAudio(&wanted, NULL) < 0) {
		fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
		return(-1);
	}

	audio_pos = audio_chunk;

	/* Let the callback function play the audio chunk */
	SDL_PauseAudio(0);

	/* Do some processing */
	SDL_Delay(4000);

	SDL_CloseAudio();
	SDL_Quit();
	return 0;
}

