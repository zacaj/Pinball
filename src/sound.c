/*
 * sound.c
 *
 *  Created on: Jul 17, 2014
 *      Author: zacaj
 */

#include <stdint.h>

#ifdef SOUND
#ifndef SDL
#include "stm32f30x.h"
#include "stm32f30x_gpio.h"
#include "stm32f30x_conf.h"
#include "stm32f3_discovery.h"
#endif
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#ifndef SDL
#include "sound.h"
#include "sound.raw"
#include "timer.h"
#include "io.h"
const IOPin LATCH = { bA, P9 };
const IOPin CLOCK = { bD, P3 };
const IOPin DATA = { bB, P13 };
#endif

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

int a = 0;
double tone = 115.50;
const double rate = 8000;
uint8_t s;
double t = 0;
Waveform w = { 1, 0, 0, { { 0, 0 }, { .01, 0 }, { .05, 1 }, { .07, .08 }, { .28, .07 }, { .4, 0 }, { 1, 0 } } };
double lastAt = 0;
double lastReset = 0;
#define NOLD 9000
int old[NOLD];
int at = NOLD;
int curSound = 0;
int softLeft = 0;
int lastSoftLeft = -1;
int lastSound = -1;
uint32_t audio(void *data)
{
	//setOutDirect(LATCH, 0);
	//double v = 1;//((w.points[at + 1].y - w.points[at].y)*((t - lastAt) / (w.points[at + 1].x - w.points[at].x)) + w.points[at].y);
	//s = ((sin(t*2.0*3.14159*tone)*v + 1)) * 127;

	//s = (bsin(t*2.0*3.14159*tone) + 1) * 127;
//if(tone==215)
	int sound=0;
	if (a >= 0) {
		if (softLeft == 0)
			sound = hard[curSound].data[a];
		else
			sound = soft[curSound].data[a];
	};
	int S = sound + old[(at-NOLD/12) % NOLD];
	old[at % NOLD] = S / 3;
	//S /= 2;
	if (S > 128)
		S = 128;
	if (S < -127)
		S = -127;
	s = Uint8(S+127);

/*else s=0;
	if(a%((int)rate)==0) {
		if(tone==115.5)
			tone=215;
		else tone=115.5;
	}
	/*uint16_t d = 28672 | (s << 4);
	uint16_t i=1<<15;
	for (;i!=0;i>>=1)
	{
		setOutDirect(CLOCK, 0);
		setOutDirect(DATA, d & i);
		setOutDirect(CLOCK, 1);
	}*/
	//printf("%i,%i\n", sound,s);
	GPIO_Write(bB,s<<8);
	//if(a%(int)rate==0)
	//	STM_EVAL_LEDToggle(LED3);
	//uint16_t d = 28672 | (s << 4);
	//SPI_I2S_SendData16(SPI2,d);
	//setOutDirect(LATCH, 1);
	a++;
	if ((softLeft == 0 && a >= hard[curSound].length) || (softLeft != 0 && a >= soft[curSound].length))
	{
		a = 0;
		if (softLeft == 1)//finished playing a hard
		{
			curSound = rand() % 3;
			if (curSound == 1)
				curSound = 0;
			if (curSound==lastSound)
				curSound = rand() % 3;
			lastSound = curSound;
			softLeft--;
		}
		else {
			curSound = rand() % 3;
			if (curSound==lastSound)
				curSound = rand() % 3;
			lastSound = curSound;
			if (softLeft == 0) {
				softLeft = (rand() % 2) + (lastSoftLeft==1?2:1) +rand()%4==0;
				lastSoftLeft = softLeft;
			}
			else
				softLeft--;
		}

	}
	at++; 
	t += 1.0 / rate;
	/*if (t - lastReset > w.points[(at+1)].x)
	{
		lastAt = w.points[at+1].x+lastReset;
		at++;
		if (at >= 6)
		{
			lastReset += w.points[6].x;
			at = 0;
		}
	}*/
	return 0;
}

/////////////////////////////////////
void initSound() {
	for (int i = 0; i < NOLD; i++)
		old[i] = 0;
	/*initOutput(LATCH);
	initOutput(CLOCK);
	initOutput(DATA);
	setOut(LATCH, 1);
	setOut(CLOCK, 0);*/
#ifndef SDL
	GPIO_InitTypeDef init;
	init.GPIO_Mode = GPIO_Mode_OUT;
	init.GPIO_OType = GPIO_OType_PP;
	init.GPIO_Pin = GPIO_Pin_All;
	init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(bB, &init);

	/*RCC_PCLK2Config(RCC_HCLK_Div2);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	{
		GPIO_InitTypeDef GPIO_InitStructure;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_NOPULL;
		GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
		GPIO_Init(bB, &GPIO_InitStructure);
	}
	GPIO_PinAFConfig(bB,GPIO_Pin_13,GPIO_AF_5);
	GPIO_PinAFConfig(bB,GPIO_Pin_15,GPIO_AF_5);

	SPI_InitTypeDef   SPI_InitStructure;
	SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI2, &SPI_InitStructure);
	SPI_Cmd(SPI2, ENABLE);*/

	callFuncInCustom(audio, 72000000.0 / rate / 1000 - 1, 1000 - 1, NULL);
#endif
	//callFuncInCustom(audio,8,1125,NULL);
	//callFuncInCustom(audio,16,1125,NULL);
	//callFuncInCustom(audio,500,23,NULL);
	//callFuncInCustom(audio,1623,,NULL);
}

#else
void initSound() {}
#endif
