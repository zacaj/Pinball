/*
 * io.c
 *
 *  Created on: Jul 16, 2014
 *      Author: zacaj
 */


#include <stdint.h>
#include "stm32f30x.h"
#include "stm32f30x_gpio.h"
#include "stm32f30x_conf.h"
#include "stm32f3_discovery.h"
#include <stdio.h>
#include "io.h"
#include "timer.h"
#include <string.h>
#include <stdlib.h>

uint8_t mInputState[nMultiInput];
uint8_t mLEDState[6];
uint8_t LED_Dirty=0;
#define nLED 48
uint8_t heldRelayState[nHeldRelay];
uint32_t LedState[nLED];

void initInput(IOPin pin, GPIOPuPd_TypeDef def)
{
	if((int)pin.bank<11)
		return;
	GPIO_InitTypeDef init;
	init.GPIO_Mode = GPIO_Mode_IN;
	init.GPIO_OType = GPIO_OType_PP;
	init.GPIO_Pin = pin.pin;
	init.GPIO_PuPd = def;

	GPIO_Init(pin.bank, &init);
}

void initOutput(IOPin pin)
{
	if((int)pin.bank<11)
		return;
	GPIO_InitTypeDef init;
	init.GPIO_Mode = GPIO_Mode_OUT;
	init.GPIO_OType = GPIO_OType_PP;
	init.GPIO_Pin = pin.pin;
	init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(pin.bank, &init);
}

inline void setOutDirect(IOPin pin, uint32_t value)
{
	GPIO_WriteBit(pin.bank, pin.pin, value);
}

void setOut(IOPin pin, uint32_t value)
{
	if((int)pin.bank<11)
	{
		if((int)pin.bank>=5 && (int)pin.bank<=10)
		{
			if((mLEDState[(int)pin.bank-5]&1<<pin.pin)?1:0!=value)
			{
				LED_Dirty=1;
				if(value)
					mLEDState[(int)pin.bank-5]|=1<<pin.pin;
				else
					mLEDState[(int)pin.bank-5]&= ~(1 << pin.pin);
			}
		}
		return;
	}

}

uint8_t getInDirect(IOPin pin)
{
	return GPIO_ReadInputDataBit(pin.bank, pin.pin);
}

uint8_t getIn(IOPin pin)
{
	if((int)pin.bank<11)
	{
		if((int)pin.bank>=1 && (int)pin.bank<=4)
			return (mInputState[(int)pin.bank-1]&(1<<pin.pin))?1:0;
		return 0;
	}
	return GPIO_ReadInputDataBit(pin.bank, pin.pin);
}

void initIOs()
{
	for(int i=0;i<4;i++)
		//for(int j=0;j<16;j++)
			mInputState[i]=0;
	for(int i=0;i<nHeldRelay;i++)
	{
		heldRelayState[i]=0;
		initOutput(heldRelays[i]);
	}

	//for(int i=0;i<nLED;i++)
	//	LedState[i]=0;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOE, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOF, ENABLE);

	initOutput(HOLD);
	initOutput(BALL_SHOOT);
	initOutput(LEFT_DROP_RESET);
	initOutput(RIGHT_DROP_RESET);
	initOutput(TOP_DROP_RESET);
	initOutput(FIVE_DROP_RESET);
	initOutput(LEFT_CAPTURE_EJECT);
	initOutput(RIGHT_CAPTURE_EJECT);
	initOutput(TOP_CAPTURE_EJECT);
	for(int i=0;i<nMultiInput;i++)
	{
		initOutput(MULTI_IN_LATCH[i]);
		initOutput(MULTI_IN_CLOCK[i]);
		initInput(MULTI_IN_DATA[i],PULL_DOWN);
	}
	for(int i=0;i<3;i++)
		for(int j=0;j<3;j++)
			initInput(DROP_TARGET[i][j],NO_PULL);
	for(int i=0;i<4;i++)
	{
		initOutput(SCORE[i]);
		initOutput(BONUS[i]);
		initInput(SCORE_ZERO[i],NO_PULL);
		initInput(BONUS_ZERO[i],NO_PULL);
		initInput(LANES[i],PULL_DOWN);
		initInput(RED_TARGET[i],PULL_DOWN);
	}
	for(int i=0;i<5;i++)
		initInput(FIVE_TARGET[i],NO_PULL);

	initInput(LEFT_CAPTURE,NO_PULL);
	initInput(RIGHT_CAPTURE,NO_PULL);
	initInput(TOP_CAPTURE,NO_PULL);
	initInput(BALL_OUT,NO_PULL);
	initInput(BALL_LOADED,NO_PULL);
	initInput(START,NO_PULL);
	initInput(CAB_LEFT,NO_PULL);
	initInput(CAB_RIGHT,NO_PULL);
	initInput(LEFT_FLIPPER,PULL_DOWN);
	initInput(RIGHT_FLIPPER,PULL_DOWN);
	initInput(LEFT_BLOCK,PULL_DOWN);
	initInput(RIGHT_BLOCK,PULL_DOWN);
	initInput(ACTIVATE_TARGET,PULL_DOWN);
	initInput(LEFT_POP,PULL_DOWN);
	initInput(RIGHT_POP,PULL_DOWN);
	initInput(BUMPER,PULL_DOWN);
	initInput(ROTATE_ROLLOVER,PULL_DOWN);


}

void updateIOs()
{
	//updateSlowInputs();
	/*
	for(int i=0;i<nLED;i++)
	{
		uint8_t oldState=mLEDState[i/8]&=1<<(i%8);
		if(LedState[i]>0 && LedState[i]<4294967295)
			LedState[i]+=10000;
		if((LedState[i]==0 || (LedState[i]>4294967295/2 && LedState[i]<4294967295)) && oldState)
		{
			mLEDState[i/8]&= ~(1 << i%8);
			LED_Dirty=1;
		}
		else if((LedState[i]==4294967295 || (LedState[i]<=4294967295/2 && LedState[i]>0)) && !oldState)
		{
			mLEDState[i/8]|= (1 << i%8);
			LED_Dirty=1;
		}
	}
	if(LED_Dirty)
	{
		for(int j=0;j<6;j++)
			for(int i=0;i<8;i++)
			{
				setOutDirect(GPIOD,LED_DATA,mLEDState[j] & 1<<i);
				setOutDirect(GPIOD,LED_CLOCK,1);
				setOutDirect(GPIOD,LED_CLOCK,0);
			}
		setOutDirect(GPIOD,LED_DATA,0);
		setOutDirect(GPIOD,LED_LATCH,1);
		setOutDirect(GPIOD,LED_LATCH,0);
		LED_Dirty=0;
	}*/
}

void updateSlowInputs()
{
	for(int i=0;i<nMultiInput;i++)
	{
		uint8_t in=0;
		setOutDirect(MULTI_IN_LATCH[i],0);
		int i;
		for(i=0;i<8;i++)
		{
			setOutDirect(MULTI_IN_CLOCK[i],0);
			in<<=1;
			in|=getInDirect(MULTI_IN_DATA[i]);
			setOutDirect(MULTI_IN_CLOCK[i],1);
		}
		setOutDirect(MULTI_IN_LATCH[i],1);
		mInputState[i]=in;
	}
}

//uint32_t lastSolenoidFiringTime= 0;

uint32_t turnOffSolenoid(IOPin *pin)
{
	setOut(*pin,0);
	free(pin);
	return 1;
}

void fireSolenoid(IOPin pin)
{
	fireSolenoidFor(pin, 90);
}

void fireSolenoidFor(IOPin pin, uint32_t ms)
{
	//while(msElapsed<lastSolenoidFiringTime+50);
	setOut(pin, 1);
	callFuncIn_s(turnOffSolenoid, ms, memcpy(malloc(sizeof(IOPin)), &pin,sizeof(IOPin)));
}

void setLED(uint8_t index,uint8_t state)
{
	switch(state)
	{
	case OFF:
		LedState[index]=0;
		break;
	case ON:
		LedState[index]=4294967295;
		break;
	case FLASHING:
		LedState[index]+=3;
		break;
	}
}

uint8_t getLED(uint8_t index)
{
	if(LedState[index]==0)
		return OFF;
	if(LedState[index]==4294967295)
		return ON;
	return FLASHING;
}

void setHeldRelay(int n,uint8_t state)
{
	heldRelayState[n]=state;
	if(state)
	{
		fireSolenoidFor(heldRelays[n],20);
	}
	else
	{
		fireSolenoidFor(HOLD,20);
		for(int i=0;i<nHeldRelay;i++)
			if(heldRelayState[i])
				fireSolenoidFor(heldRelays[i],20);
	}
}
