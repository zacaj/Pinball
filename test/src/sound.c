/*
 * sound.c
 *
 *  Created on: Jul 17, 2014
 *      Author: zacaj
 */

#include <stddef.h>
#include "stm32f30x.h"
#include "stm32f30x_gpio.h"
#include "stm32f30x_conf.h"
#include "stm32f3_discovery.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "sound.h"
#include "timer.h"
#include "io.h"

const IOPin LATCH = {bA,P9};
const IOPin CLOCK = {bD,P3};
const IOPin DATA = {bB,P13};
///////////////////////////////////////////

typedef struct
{
	double x, y;
} vec2;

struct Waveform_t
{
	double volume; //0-1
	double lead, after;
	vec2 points[7];
};
typedef struct Waveform_t Waveform;

Waveform initWaveform(uint8_t nPoints)
{
	Waveform w;// *w = (Waveform*)malloc(sizeof(Waveform));
	w.volume = 1;
	w.lead = w.after = 0;
	return w;
}

struct Note_t
{
	struct Note_t *next;
	double lead, after;
	double volume; //0-1
	Waveform wave;
};

typedef struct
{
	double volume;

} Channel;

#define NUM_CHANNEL 8
Channel channels[NUM_CHANNEL];

uint32_t a = 0;
double tone = 115.50;
double rate = 8000;
uint8_t s;
double t = 0;
Waveform w = { 1, 0, 0, { { 0, 0 }, { .01, 0 }, { .05, 1 }, { .07, .08 }, { .28, .07 }, { .4, 0 }, { 1, 0 } } };
int at = 0;
double lastAt = 0;
double lastReset = 0;

uint32_t audio(void *data)
{
	setOut(CLOCK, 0);
	setOut(LATCH, 0);
	double v = ((w.points[at + 1].y - w.points[at].y)*((t - lastAt) / (w.points[at + 1].x - w.points[at].x)) + w.points[at].y);
	s = ((sin(t*2.0*3.14159*tone)*v + 1)) * 127;

	//s = (sin(t*2.0*3.14159*tone) + 1) * 127;

	uint16_t d = 28672 | (s << 4);
	for (int i = 15; i >= 0; i--)
	{
		setOut(CLOCK, 0);
		setOut(DATA, (d & 1 << i) ? 1 : 0);
		setOut(CLOCK, 1);
	}
	setOut(LATCH, 1);
	a++;
	t += 1.0 / rate;
	if (t - lastReset > w.points[(at+1)].x)
	{
		lastAt = w.points[at+1].x+lastReset;
		at++;
		if (at >= 6)
		{
			lastReset += w.points[6].x;
			at = 0;
		}
	}
	return 0;
}

/////////////////////////////////////
void initSound() {
	initOutput(LATCH);
	initOutput(CLOCK);
	initOutput(DATA);
	setOut(LATCH, 1);
	setOut(CLOCK, 0);

	callFuncInCustom(audio,72000000/rate,1,NULL);
}
