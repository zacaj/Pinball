/**
*****************************************************************************
**
**  File        : main.c
**
**  Abstract    : main function.
**
**  Functions   : main
**
**  Environment : Atollic TrueSTUDIO(R)
**
**  Distribution: The file is distributed “as is,” without any warranty
**                of any kind.
**
**  (c)Copyright Atollic AB.
**  You may use this file as-is or modify it according to the needs of your
**  project. This file may only be built (assembled or compiled and linked)
**  using the Atollic TrueSTUDIO(R) product. The use of this file together
**  with other tools than Atollic TrueSTUDIO(R) is not permitted.
**
*****************************************************************************
*/

/* Includes */
#include <stdint.h>
#include "stm32f30x.h"
#include "stm32f30x_gpio.h"
#include "stm32f30x_conf.h"
#include "stm32f3_discovery.h"
#include <stdio.h>
#include <math.h>
#include "timer.h"
#include "io.h"
#include "sound.h"
/* Private typedef */

/* Private define  */

/* Private macro */

/* Private variables */
uint32_t buttonState=0;

/* Private function prototypes */
/* Private functions */

/*
 * no out: PF0,1
 * half out: PE8-15 (led)
 *
 * check:
 * PE0-5 PC14,15 PB3,6,7 PA5-7,11-14
 */

IOPin pins[8]={
		{bC,P7},
		{bD,P14},
		{bD,P12},
		{bD,P10},
		{bD,P8},
		{bB,P14},
		{bB,P12},
		{bB,P10},
		/*{GPIOA,GPIO_Pin_5},//out	in
		{GPIOA,GPIO_Pin_6},//out	in
		{GPIOA,GPIO_Pin_7},//out	in
		{GPIOA,GPIO_Pin_11},//0/0
		{GPIOA,GPIO_Pin_12},//0/0
		{GPIOA,GPIO_Pin_13},//1.5/3  3/3 break?
		{GPIOA,GPIO_Pin_14},//0/0	in  break?

		{GPIOB,GPIO_Pin_3},//0/0
		{GPIOB,GPIO_Pin_6},//3/3
		{GPIOB,GPIO_Pin_7},//3/3	3/3

		{GPIOC,GPIO_Pin_14},//0/0
		{GPIOC,GPIO_Pin_15},//0/0

		{GPIOE,GPIO_Pin_0},//0/.4	in
		{GPIOE,GPIO_Pin_1},//0/.4	in
		{GPIOE,GPIO_Pin_2},//2.4/3	3/3
		{GPIOE,GPIO_Pin_3},//out	in
		{GPIOE,GPIO_Pin_4},//0/.4	in
		{GPIOE,GPIO_Pin_5},//0/0	in*/
};
/**
 * 62 io
 * 4 i
 * 8 led
 * 1 button
 * 10 bad
 *
**===========================================================================
**
**  Abstract: main program
**
**===========================================================================
*/

uint32_t ii;

//return 1 to disable timer, 0 to repeat
uint32_t disableLight(void *d)
{
	setOut(pins[(int)d],0);
	STM_EVAL_LEDOff(LED3+(int)d);
	return 1;
}
void switchPlayerRelay(int n)
{
	for(int i=0;i<4;i++)
		heldRelayState[PLAYER_ENABLE[i]]=0;
	setHeldRelay(n-1,1);
}
extern uint8_t mInputState[];
int main(void)
{

	/* Example use SysTick timer and read System core clock */
	SysTick_Config(72000);  /* 1  ms if clock frequency 72 MHz */
	initTimers();
	initIOs();
	//initSound();
	STM_EVAL_PBInit(BUTTON_USER,BUTTON_MODE_GPIO);
	int i;

	SystemCoreClockUpdate();
	for(i=0;i<8;i++)
		STM_EVAL_LEDInit(LED3+i);
	STM_EVAL_LEDToggle(LED3);
	ii = 0;
	int iii=0;
	int iiii=0;
	switchPlayerRelay(1);
	while (1)
	{
		if (msTicks>500)
		{
			msTicks=0;
			if(ii>=4)
			{
				ii=0;
				iii++;
				if(iii>=4)
					iii=0;
				switchPlayerRelay(iii+1);
			}
			STM_EVAL_LEDOff(LED3+ii);
			//setOut(pins[ii],1);
			//callFuncIn(disableLight,1000,ii);
			fireSolenoid(SCORE[ii]);
			ii++;
			STM_EVAL_LEDOn(LED3+ii);
		}
		updateIOs();
		/*uint8_t in=1;
		if(iiii!=4)
			in=mInputState[iiii];
		for(int i=0;i<8;i++)
		{
			if(in& 1<<i)
				STM_EVAL_LEDOn(LED3+i);
			else
				STM_EVAL_LEDOff(LED3+i);
		}*/
		if(buttonState!=STM_EVAL_PBGetState(BUTTON_USER))
		{
			buttonState=!buttonState;
			if(buttonState)
			{
				/*updateSlowInputs();
				for(int i=0;i<4;i++)
				{
					switchPlayerRelay(i);
					for(int j=0;j<4;j++)
					{
						while (getIn(SCORE_ZERO[j]))
						{
							fireSolenoid(SCORE[j]);
							updateSlowInputs();
						}
					}
				}*/
				iiii++;
				if(iiii>4)
					iiii=0;
			}
		}
	}
	return 0;
}
