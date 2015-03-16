/*
 * timer.c
 *
 *  Created on: Jul 14, 2014
 *      Author: zacaj
 */

#include <stdint.h>
#include "stm32f30x.h"
#include "stm32f30x_gpio.h"
#include "stm32f30x_conf.h"
#include "stm32f3_discovery.h"
#include <stdio.h>
#include <stdlib.h>
#include "timer.h"


uint32_t (*timerFuncs[11])(void*);
uint8_t active[10];
void *timerData[10];
uint32_t setting[10];
uint32_t msTicks = 0,msElapsed=0;

TIM_TypeDef* timerIds[]={
		  TIM2,
		  TIM1,
		  TIM3,
		  TIM4,
		  TIM6,
		  TIM7,
		  TIM8,
		  TIM15,
		  TIM16,
		  TIM17
};
void TIM_IRQHandler()
{
	for(int i=0;i<10;i++)
		if (TIM_GetITStatus(timerIds[i], TIM_IT_Update) != RESET)
		{
			TIM_ClearITPendingBit(timerIds[i], TIM_IT_Update);
			if(active[i])
			{
				if(timerFuncs[i]!=NULL)
				{
					if(timerFuncs[i](timerData[i])) {
						active[i]=0;
						stopTimer(i);
					}
				}
			}
		}
}
void TIM1_BRK_TIM15_IRQHandler(void) {	TIM_IRQHandler(); }
void TIM8_BRK_IRQHandler(void) {	TIM_IRQHandler(); }
void TIM2_IRQHandler(void) {	TIM_IRQHandler(); }
void TIM3_IRQHandler(void) {	TIM_IRQHandler(); }
void TIM4_IRQHandler(void) {	TIM_IRQHandler(); }
void TIM7_IRQHandler(void) {	TIM_IRQHandler(); }
void TIM6_DAC_IRQHandler(void) {	TIM_IRQHandler(); }
void TIM8_CC_IRQHandler(void) {	TIM_IRQHandler(); }
void TIM8_TRG_COM_IRQHandler(void) {	TIM_IRQHandler(); }
void TIM8_UP_IRQHandler(void) {	TIM_IRQHandler(); }
void TIM1_CC_IRQHandler(void) {	TIM_IRQHandler(); }
void TIM1_TRG_COM_TIM17_IRQHandler(void) {	TIM_IRQHandler(); }
void TIM1_UP_TIM16_IRQHandler(void) {	TIM_IRQHandler(); }



void startTimer(uint32_t n)
{
	TIM_ClearITPendingBit(timerIds[n], TIM_IT_Update);
	TIM_SetCounter(timerIds[n], 1);
	TIM_Cmd(timerIds[n], ENABLE);
	active[n]=1;
}

void setTimer(uint32_t n, uint32_t ms)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 2 * (ms * (1)) - 1; // 1 MHz down to 1 KHz (1 ms)
	TIM_TimeBaseStructure.TIM_Prescaler = 36000 - 1; // 24 MHz Clock down to 2 KHz (adjust per your clock)
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(timerIds[n], &TIM_TimeBaseStructure);
	setting[n]=ms;
}

void setTimerCustom(uint32_t n, uint32_t prescalar, uint32_t period) {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = period; // 1 MHz down to 1 KHz (1 ms)
	TIM_TimeBaseStructure.TIM_Prescaler = prescalar; // 24 MHz Clock down to 1 MHz (adjust per your clock)
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(timerIds[n], &TIM_TimeBaseStructure);
	setting[n]=-1;
}
void stopTimer(uint32_t n)
{
	TIM_Cmd(timerIds[n], DISABLE);
}

void initTimers()
{
	int i;
	for (i = 0; i < 10; i++)
	{
		timerFuncs[i] = NULL;
		timerData[i]=NULL;
		active[i]=0;
	}
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM15, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM16, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM17, ENABLE);
	}
	const IRQn_Type IRQns[13] =
	{ TIM1_CC_IRQn, TIM2_IRQn, TIM3_IRQn, TIM4_IRQn, TIM6_DAC_IRQn, TIM7_IRQn,
			TIM8_BRK_IRQn, TIM8_UP_IRQn, TIM8_TRG_COM_IRQn, TIM8_CC_IRQn,
			TIM1_BRK_TIM15_IRQn, TIM1_UP_TIM16_IRQn, TIM1_TRG_COM_TIM17_IRQn };
	for (i = 0; i < 13; i++)
	{
		NVIC_InitTypeDef NVIC_InitStructure;
		/* Enable the TIM2 gloabal Interrupt */
		NVIC_InitStructure.NVIC_IRQChannel = IRQns[i];
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
	}
	for (i = 0; i < 10; i++)
	{
		TIM_ITConfig(timerIds[i], TIM_IT_Update, ENABLE);
		TIM_Cmd(timerIds[i], DISABLE);
		TIM_SetCounter(timerIds[i], 0);
	}
}

uint8_t callFuncIn(uint32_t (*func)(void*), uint32_t ms,void *data)
{
	int i = 1;
	for (; i < 10; i++)
		if (!active[i])
			break;
	if (i < 10)
	{
		timerFuncs[i] = func;
		timerData[i]=data;
		setTimer(i, ms);
		startTimer(i);
		return 1;
	}
	return 0;
}

void callFuncIn_s(uint32_t (*func)(void*), uint32_t ms, void* data)
{
	//ms=400;
	if(!callFuncIn(func,ms,data))
	{
		wait(ms);
		func(data);
		printf("xgfdhdfgh");
	}
}
uint8_t callFuncInCustom(uint32_t (*func)(void*), uint32_t prescalar, uint32_t period,void *data)
{
	int i = 1;
	for (; i < 10; i++)
		if (!active[i])
			break;
	if (i < 10)
	{
		timerFuncs[i] = func;
		timerData[i]=data;
		setTimerCustom(i, prescalar,period);
		startTimer(i);
		return 1;
	}
	return 0;
}
void wait(uint32_t ms)
{
	uint32_t start=msElapsed;
	while(msElapsed<start+ms);
}

float fast_cossin_table[]={0.000000,0.012272,0.024541,0.036807,0.049068,0.061321,0.073565,0.085797,0.098017,0.110222,0.122411,0.134581,0.146730,0.158858,0.170962,0.183040,0.195090,0.207111,0.219101,0.231058,0.242980,0.254866,0.266713,0.278520,0.290285,0.302006,0.313682,0.325310,0.336890,0.348419,0.359895,0.371317,0.382683,0.393992,0.405241,0.416430,0.427555,0.438616,0.449611,0.460539,0.471397,0.482184,0.492898,0.503538,0.514103,0.524590,0.534998,0.545325,0.555570,0.565732,0.575808,0.585798,0.595699,0.605511,0.615232,0.624860,0.634393,0.643832,0.653173,0.662416,0.671559,0.680601,0.689541,0.698376,0.707107,0.715731,0.724247,0.732654,0.740951,0.749136,0.757209,0.765167,0.773010,0.780737,0.788346,0.795837,0.803208,0.810457,0.817585,0.824589,0.831470,0.838225,0.844854,0.851355,0.857729,0.863973,0.870087,0.876070,0.881921,0.887640,0.893224,0.898674,0.903989,0.909168,0.914210,0.919114,0.923880,0.928506,0.932993,0.937339,0.941544,0.945607,0.949528,0.953306,0.956940,0.960431,0.963776,0.966976,0.970031,0.972940,0.975702,0.978317,0.980785,0.983105,0.985278,0.987301,0.989177,0.990903,0.992480,0.993907,0.995185,0.996313,0.997290,0.998118,0.998795,0.999322,0.999699,0.999925,1.000000,0.999925,0.999699,0.999322,0.998795,0.998118,0.997290,0.996313,0.995185,0.993907,0.992480,0.990903,0.989177,0.987301,0.985278,0.983105,0.980785,0.978317,0.975702,0.972940,0.970031,0.966976,0.963776,0.960431,0.956940,0.953306,0.949528,0.945607,0.941544,0.937339,0.932993,0.928506,0.923880,0.919114,0.914210,0.909168,0.903989,0.898674,0.893224,0.887640,0.881921,0.876070,0.870087,0.863973,0.857729,0.851355,0.844854,0.838225,0.831470,0.824589,0.817585,0.810457,0.803208,0.795837,0.788346,0.780737,0.773010,0.765167,0.757209,0.749136,0.740951,0.732654,0.724247,0.715731,0.707107,0.698376,0.689541,0.680601,0.671559,0.662416,0.653173,0.643831,0.634393,0.624859,0.615232,0.605511,0.595699,0.585798,0.575808,0.565732,0.555570,0.545325,0.534998,0.524590,0.514103,0.503538,0.492898,0.482184,0.471397,0.460539,0.449611,0.438616,0.427555,0.416429,0.405241,0.393992,0.382683,0.371317,0.359895,0.348419,0.336890,0.325310,0.313682,0.302006,0.290285,0.278520,0.266713,0.254866,0.242980,0.231058,0.219101,0.207111,0.195090,0.183040,0.170962,0.158858,0.146730,0.134581,0.122411,0.110222,0.098017,0.085797,0.073564,0.061321,0.049068,0.036807,0.024541,0.012271,-0.000000,-0.012272,-0.024541,-0.036807,-0.049068,-0.061321,-0.073565,-0.085797,-0.098017,-0.110222,-0.122411,-0.134581,-0.146731,-0.158858,-0.170962,-0.183040,-0.195090,-0.207111,-0.219101,-0.231058,-0.242980,-0.254866,-0.266713,-0.278520,-0.290285,-0.302006,-0.313682,-0.325310,-0.336890,-0.348419,-0.359895,-0.371317,-0.382684,-0.393992,-0.405241,-0.416430,-0.427555,-0.438616,-0.449611,-0.460539,-0.471397,-0.482184,-0.492898,-0.503538,-0.514103,-0.524590,-0.534998,-0.545325,-0.555570,-0.565732,-0.575808,-0.585798,-0.595699,-0.605511,-0.615232,-0.624860,-0.634393,-0.643832,-0.653173,-0.662416,-0.671559,-0.680601,-0.689541,-0.698376,-0.707107,-0.715731,-0.724247,-0.732654,-0.740951,-0.749136,-0.757209,-0.765167,-0.773011,-0.780737,-0.788346,-0.795837,-0.803208,-0.810457,-0.817585,-0.824589,-0.831470,-0.838225,-0.844854,-0.851355,-0.857729,-0.863973,-0.870087,-0.876070,-0.881921,-0.887640,-0.893224,-0.898674,-0.903989,-0.909168,-0.914210,-0.919114,-0.923880,-0.928506,-0.932993,-0.937339,-0.941544,-0.945607,-0.949528,-0.953306,-0.956940,-0.960431,-0.963776,-0.966977,-0.970031,-0.972940,-0.975702,-0.978317,-0.980785,-0.983106,-0.985278,-0.987301,-0.989177,-0.990903,-0.992480,-0.993907,-0.995185,-0.996313,-0.997290,-0.998118,-0.998795,-0.999322,-0.999699,-0.999925,-1.000000,-0.999925,-0.999699,-0.999322,-0.998795,-0.998118,-0.997290,-0.996313,-0.995185,-0.993907,-0.992480,-0.990903,-0.989177,-0.987301,-0.985278,-0.983105,-0.980785,-0.978317,-0.975702,-0.972940,-0.970031,-0.966976,-0.963776,-0.960431,-0.956940,-0.953306,-0.949528,-0.945607,-0.941544,-0.937339,-0.932993,-0.928506,-0.923880,-0.919114,-0.914210,-0.909168,-0.903989,-0.898674,-0.893224,-0.887640,-0.881921,-0.876070,-0.870087,-0.863973,-0.857729,-0.851355,-0.844853,-0.838225,-0.831470,-0.824589,-0.817585,-0.810457,-0.803207,-0.795837,-0.788346,-0.780737,-0.773010,-0.765167,-0.757209,-0.749136,-0.740951,-0.732654,-0.724247,-0.715731,-0.707107,-0.698376,-0.689540,-0.680601,-0.671559,-0.662416,-0.653173,-0.643831,-0.634393,-0.624859,-0.615231,-0.605511,-0.595699,-0.585798,-0.575808,-0.565732,-0.555570,-0.545325,-0.534997,-0.524590,-0.514103,-0.503538,-0.492898,-0.482184,-0.471397,-0.460539,-0.449611,-0.438616,-0.427555,-0.416429,-0.405241,-0.393992,-0.382683,-0.371317,-0.359895,-0.348419,-0.336890,-0.325310,-0.313682,-0.302006,-0.290285,-0.278520,-0.266713,-0.254865,-0.242980,-0.231058,-0.219101,-0.207111,-0.195090,-0.183040,-0.170962,-0.158858,-0.146730,-0.134581,-0.122411,-0.110222,-0.098017,-0.085797,-0.073564,-0.061321,-0.049068,-0.036807,-0.024541,-0.012271};
void _BREAK()
{
	while(1)
	{
		uint8_t anyActive=0;
		for(int i=0;i<10;i++)
			anyActive|=active[i];
		if(!anyActive)
			break;
	}

}
