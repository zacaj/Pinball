/*
 * timer.c
 *
 *  Created on: Jul 14, 2014
 *      Author: zacaj
 */

#include <stddef.h>
#include "stm32f30x.h"
#include "stm32f30x_gpio.h"
#include "stm32f30x_conf.h"
#include "stm32f3_discovery.h"
#include <stdio.h>
#include "timer.h"


uint32_t (*timerFuncs[11])();
uint8_t active[10];
void *timerData[10];
uint32_t setting[10];

TIM_TypeDef* timerIds[]={
		  TIM1,
		  TIM2,
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
#define HANDLE_(a,i) \
	if (TIM_GetITStatus(a, TIM_IT_Update) != RESET) \
	{ \
		active[i]=0; \
		if(timerFuncs[i]!=NULL) \
		{ \
			if(timerFuncs[i](timerData[i])) \
				stopTimer(i); \
			else \
				active[i]=1; \
		} \
		TIM_ClearITPendingBit(a, TIM_IT_Update); \
	}

	for(int i=0;i<10;i++)
		HANDLE_(timerIds[i],i)
	/*HANDLE_(TIM1,0)
	HANDLE_(TIM2,1)
	HANDLE_(TIM3,2)
	HANDLE_(TIM4,3)
	HANDLE_(TIM6,4)
	HANDLE_(TIM7,5)
	HANDLE_(TIM8,6)
	HANDLE_(TIM15,7)
	HANDLE_(TIM16,8)
	HANDLE_(TIM17,9)*/
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
	TIM_SetCounter(timerIds[n], 0);
	active[n]=1;
	TIM_Cmd(timerIds[n], ENABLE);
}

void setTimer(uint32_t n, uint32_t ms)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 2 * (ms * (1)) - 1; // 1 MHz down to 1 KHz (1 ms)
	TIM_TimeBaseStructure.TIM_Prescaler = 36000 - 1; // 24 MHz Clock down to 1 MHz (adjust per your clock)
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(timerIds[n], &TIM_TimeBaseStructure);
	setting[n]=ms;
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

uint8_t callFuncIn(uint32_t (*func)(), uint32_t ms,void *data)
{
	int i = 5;
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
