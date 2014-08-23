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

Solenoid HOLD=Sd(bU,P0,20);
Solenoid SCORE[4]={S(bU,P0),S(bU,P0),S(bU,P0),S(bU,P0)};
Solenoid BONUS[4]={S(bU,P0),S(bU,P0),S(bU,P0),S(bU,P0)};
Solenoid BALL_SHOOT=Sd(bU,P0,120);
Solenoid LEFT_DROP_RESET=S(bU,P0);
Solenoid RIGHT_DROP_RESET=S(bU,P0);
Solenoid TOP_DROP_RESET=S(bU,P0);
Solenoid FIVE_DROP_RESET=S(bU,P0);
Solenoid LEFT_CAPTURE_EJECT=S(bU,P0);
Solenoid RIGHT_CAPTURE_EJECT=S(bU,P0);
Solenoid TOP_CAPTURE_EJECT=S(bU,P0);
Solenoid heldRelays[nHeldRelay]={
		/*Player enable 1*/Sd(bU,P0,20),
		Sd(bU,P0,20),
		Sd(bU,P0,20),
		Sd(bU,P0,20),/*Player enable 4*/
		Sd(bU,P0,20),//ball ack
		Sd(bU,P0,20),//ball release
		Sd(bU,P0,20),//magnet
		Sd(bU,P0,20),//left block
		Sd(bU,P0,20),//right block
		Sd(bU,P0,20),//playfield disable
};

 Input DROP_TARGET[3][3]=
{
		{In(bU,P0),In(bU,P0),In(bU,P0)},
		{In(bU,P0),In(bU,P0),In(bU,P0)},
		{In(bU,P0),In(bU,P0),In(bU,P0)}
};
 Input FIVE_TARGET[5]={In(bU,P0),In(bU,P0),In(bU,P0),In(bU,P0),In(bU,P0)};
 Input LEFT_CAPTURE=In(bU,P0);
 Input RIGHT_CAPTURE=In(bU,P0);
 Input TOP_CAPTURE=In(bU,P0);
 Input SCORE_ZERO[4]={In(bU,P0),In(bU,P0),In(bU,P0),In(bU,P0)};
 Input BONUS_ZERO[4]={In(bU,P0),In(bU,P0),In(bU,P0),In(bU,P0)};
 Input BALL_OUT=In(bU,P0);
 Input BALL_LOADED=In(bU,P0);
 Input LANES[4]={In(bU,P0),In(bU,P0),In(bU,P0),In(bU,P0)};
 Input LEFT_FLIPPER=In(bU,P0);
 Input LEFT_BLOCK=In(bU,P0);
 Input RIGHT_FLIPPER=In(bU,P0);
 Input RIGHT_BLOCK=In(bU,P0);
 Input START=In(bU,P0);
 Input CAB_LEFT=In(bU,P0);
 Input CAB_RIGHT=In(bU,P0);
 Input LEFT_POP=In(bU,P0);
 Input RIGHT_POP=In(bU,P0);
 Input BUMPER=In(bU,P0);
 Input ROTATE_ROLLOVER=In(bU,P0);
 Input ACTIVATE_TARGET=In(bU,P0);
 Input RED_TARGET[4]={In(bU,P0),In(bU,P0),In(bU,P0),In(bU,P0)};


void initInput(IOPin pin, GPIOPuPd_TypeDef def)
{
	if(pin.bank==bU)
		return;
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
	if(pin.bank==bU)
		return;
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
	if(pin.bank==bU)
		return;
	GPIO_WriteBit(pin.bank, pin.pin, value);
}

void setOut(IOPin pin, uint32_t value)
{
	if(pin.bank==bU)
		return;
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
	if(pin.bank==bU)
		return 0;
	return GPIO_ReadInputDataBit(pin.bank, pin.pin);
}

uint8_t getIn(IOPin pin)
{
	if(pin.bank==bU)
		return 0;
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
		initOutput(heldRelays[i].pin);
	}

	for(int i=0;i<nLED;i++)
		LedState[i]=0;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOE, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOF, ENABLE);

	initOutput(HOLD.pin);
	initOutput(BALL_SHOOT.pin);
	initOutput(LEFT_DROP_RESET.pin);
	initOutput(RIGHT_DROP_RESET.pin);
	initOutput(TOP_DROP_RESET.pin);
	initOutput(FIVE_DROP_RESET.pin);
	initOutput(LEFT_CAPTURE_EJECT.pin);
	initOutput(RIGHT_CAPTURE_EJECT.pin);
	initOutput(TOP_CAPTURE_EJECT.pin);
	initOutput(MULTI_IN_LATCH);
	initOutput(MULTI_IN_CLOCK);
	setOutDirect(MULTI_IN_CLOCK,0);
	setOutDirect(MULTI_IN_LATCH,1);
	initOutput(LED_LATCH);
	setOutDirect(LED_LATCH,0);
	initOutput(LED_DATA);
	setOutDirect(LED_DATA,0);
	initOutput(LED_CLOCK);
	setOutDirect(LED_CLOCK,0);

	for(int i=0;i<nMultiInput;i++)
	{
		initInput(MULTI_IN_DATA[i],PULL_DOWN);
	}
	for(int i=0;i<3;i++)
		for(int j=0;j<3;j++)
			initInput(DROP_TARGET[i][j].pin,NO_PULL);
	for(int i=0;i<4;i++)
	{
		initOutput(SCORE[i].pin);
		initOutput(BONUS[i].pin);
		initInput(SCORE_ZERO[i].pin,NO_PULL);
		initInput(BONUS_ZERO[i].pin,NO_PULL);
		initInput(LANES[i].pin,PULL_DOWN);
		initInput(RED_TARGET[i].pin,PULL_DOWN);
	}
	for(int i=0;i<5;i++)
		initInput(FIVE_TARGET[i].pin,NO_PULL);

	initInput(LEFT_CAPTURE.pin,NO_PULL);
	initInput(RIGHT_CAPTURE.pin,NO_PULL);
	initInput(TOP_CAPTURE.pin,NO_PULL);
	initInput(BALL_OUT.pin,NO_PULL);
	initInput(BALL_LOADED.pin,NO_PULL);
	initInput(START.pin,NO_PULL);
	initInput(CAB_LEFT.pin,NO_PULL);
	initInput(CAB_RIGHT.pin,NO_PULL);
	initInput(LEFT_FLIPPER.pin,PULL_DOWN);
	initInput(RIGHT_FLIPPER.pin,PULL_DOWN);
	initInput(LEFT_BLOCK.pin,PULL_DOWN);
	initInput(RIGHT_BLOCK.pin,PULL_DOWN);
	initInput(ACTIVATE_TARGET.pin,PULL_DOWN);
	initInput(LEFT_POP.pin,PULL_DOWN);
	initInput(RIGHT_POP.pin,PULL_DOWN);
	initInput(BUMPER.pin,PULL_DOWN);
	initInput(ROTATE_ROLLOVER.pin,PULL_DOWN);


}

void updateInput(Input* in)
{
	uint8_t state=getIn(in->pin);
	in->pressed=0;
	in->released=0;
	if(state!=in->state && msElapsed-in->lastChange>30)
	{
		in->state=state;
		in->lastChange=msElapsed;
		if(state)
			in->pressed=1;
		else
			in->released=1;
	}
}

void updateIOs()
{
	updateSlowInputs();
	{
		for(int i=0;i<3;i++)
			for(int j=0;j<3;j++)
				updateInput(&DROP_TARGET[i][j]);
		for(int i=0;i<5;i++)
			updateInput(&FIVE_TARGET[i]);
		for(int i=0;i<4;i++)
		{
			updateInput(&SCORE_ZERO[i]);
			updateInput(&BONUS_ZERO[i]);
			updateInput(&LANES[i]);
			updateInput(&RED_TARGET[i]);
		}
		updateInput(&LEFT_CAPTURE);
		updateInput(&RIGHT_CAPTURE);
		updateInput(&TOP_CAPTURE);
		updateInput(&BALL_OUT);
		updateInput(&BALL_LOADED);
		updateInput(&START);
		updateInput(&CAB_LEFT);
		updateInput(&CAB_RIGHT);
		updateInput(&LEFT_FLIPPER);
		updateInput(&RIGHT_FLIPPER);
		updateInput(&LEFT_BLOCK);
		updateInput(&RIGHT_BLOCK);
		updateInput(&ACTIVATE_TARGET);
		updateInput(&LEFT_POP);
		updateInput(&RIGHT_POP);
		updateInput(&BUMPER);
		updateInput(&ROTATE_ROLLOVER);
	}

	for(int i=0;i<nLED;i++)
	{
		uint8_t oldState=mLEDState[i/8]& 1<<(i%8);
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
		setOutDirect(LED_CLOCK,0);
		for(int j=0;j<6;j++)
			for(int i=0;i<8;i++)
			{
				setOutDirect(LED_DATA,mLEDState[j] & 1<<i);//
				setOutDirect(LED_CLOCK,1);
				setOutDirect(LED_CLOCK,0);
			}
		setOutDirect(LED_DATA,0);
		setOutDirect(LED_LATCH,1);
		setOutDirect(LED_LATCH,0);
		LED_Dirty=0;
	}
}

void updateSlowInputs()
{
	setOutDirect(MULTI_IN_LATCH,0);
	uint8_t in[nMultiInput];
	for(int i=0;i<nMultiInput;i++)
		in[i]=0;
	for(int i=0;i<8;i++)
	{
		setOutDirect(MULTI_IN_CLOCK,0);
		for(int j=0;j<nMultiInput;j++)
		{
			in[j]<<=1;
			in[j]|=getInDirect(MULTI_IN_DATA[j]);
		}
		setOutDirect(MULTI_IN_CLOCK,1);
	}
	for(int i=0;i<nMultiInput;i++)
		mInputState[i]=in[i];
	setOutDirect(MULTI_IN_LATCH,1);
}

//uint32_t lastSolenoidFiringTime= 0;

uint32_t turnOffSolenoid(Solenoid *s)
{
	setOut(s->pin,0);
	//free(pin);
	s->lastFired=msElapsed;
	return 1;
}

void fireSolenoid(Solenoid *s)
{
	fireSolenoidFor(s, s->onTime);
}

void fireSolenoidFor(Solenoid *s, uint32_t ms)
{
	if(s->offTime+s->lastFired>msElapsed)
		return;
	//while(msElapsed<lastSolenoidFiringTime+50);
	setOut(s->pin, 1);
	callFuncIn_s(turnOffSolenoid, ms,s);
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
		fireSolenoidFor(&heldRelays[n],20);
	}
	else
	{
		fireSolenoidFor(&HOLD,20);
		for(int i=0;i<nHeldRelay;i++)
			if(heldRelayState[i])
				fireSolenoidFor(&heldRelays[i],20);
	}
}
