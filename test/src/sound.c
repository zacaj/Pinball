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

#include "sound.h"
#include "timer.h"
#include "io.h"

const IOPin LATCH = {bA,P9};
const IOPin CLOCK = {bD,P3};
const IOPin DATA = {bB,P13};

uint32_t a=0;
double tone=220.00;
double rate=8000;
uint32_t audio(void *data)
{
	setOut(CLOCK,0);
	setOut(LATCH,0);
	uint8_t s=(sin(((double)a)*2.0*3.14159/8000.0*tone )+1)*127;
	if(a%4000==0)
		tone=tone==220.0?1046.50:220;
	uint16_t d=28672|(s<<4);
	for(int i=15;i>=0;i--)
	{
		setOut(CLOCK,0);
		setOut(DATA,(d& 1<<i)?1:0);
		setOut(CLOCK,1);
	}
	setOut(LATCH,1);
	a++;
	return 0;
}
void initSound() {
	initOutput(LATCH);
	initOutput(CLOCK);
	initOutput(DATA);
	setOut(LATCH, 1);
	setOut(CLOCK, 0);

	callFuncInCustom(audio,72000000/rate,1,NULL);
}
