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

#define _GPIO_WriteBit(b,p,v) \
	if(!v) ((b))->BRR = ((p)); \
else ((b))->BSRR = ((p));

#define _GPIO_ReadInputDataBit(b,p) \
	(( ( ((b))->IDR & ((p)) ) != 0 ))

uint8_t mInputState[nMultiInput];
typedef struct
{
	uint8_t state;
	uint32_t flashAt,flashMax;
	void *data;
	uint8_t pwm;
	uint8_t (*pwmFunc)(void*);
}LedState;
uint8_t mLEDState[6];
uint8_t LED_Dirty=0;
uint8_t heldRelayState[nHeldRelay];
uint32_t lastHeldRelayOnTime[nHeldRelay];
LedState ledState[nLED];

Solenoid HOLD=Sd(bA,P15,20);
Solenoid SCORE[4]={S(bB,P4),S(bD,P7),S(bD,P5),S(bD,P3)};
Solenoid BONUS[4]={S(bF,P10),S(bC,P13),S(bB,P9),S(bB,P5)};
Solenoid BALL_SHOOT=Sds(bC,P1,120,500);
Solenoid LEFT_DROP_RESET=Sds(bC,P8,120,700);
Solenoid RIGHT_DROP_RESET=Sds(bF,P6,120,700);
Solenoid TOP_DROP_RESET=Sds(bD,P6,120,700);
Solenoid FIVE_DROP_RESET=Sds(bD,P4,120,700);
Solenoid LEFT_CAPTURE_EJECT=Sds(bD,P2,120,700);
Solenoid RIGHT_CAPTURE_EJECT=Sds(bA,P8,120,700);
Solenoid TOP_CAPTURE_EJECT=Sds(bD,P0,120,700);
Solenoid heldRelays[nHeldRelay]={
		/*Player enable 1*/Sd(bD,P1,20),
		Sd(bC,P12,20),
		Sd(bC,P10,20),
		Sd(bB,P8,20),/*Player enable 4*/
		Sd(bC,30,20),//ball shoot enable
		Sd(bA,P1,20),//ball release
		Sd(bA,P10,20),//magnet
		Sd(bA,P3,20),//left block
		Sd(bF,P4,20),//right block
		Sd(bC,P11,20),//playfield disable
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
 Input BALL_OUT=In(bB,P10);
 Input BALL_LOADED=In(bB,P12);
 Input BALLS_FULL=In(bB,P14);
 Input LANES[4]={In(bD,P8),In(bD,P10),In(bD,P12),In(bD,P14)};
 Input LEFT_FLIPPER=In(bU,P0);
 Input LEFT_BLOCK=In(bU,P0);
 Input RIGHT_FLIPPER=In(bU,P0);
 Input RIGHT_BLOCK=In(bU,P0);
 Input START=In(bU,P0);
 Input CAB_LEFT=In(bU,P0);
 Input CAB_RIGHT=In(bU,P0);
 Input LEFT_POP=In(bB,P11);
 Input RIGHT_POP=In(bB,P13);
 Input BUMPER=In(bC,P7);
 Input ROTATE_ROLLOVER=In(bB,P15);
 Input ACTIVATE_TARGET=In(bD,P9);
 Input RED_TARGET[4]={In(bD,P11),In(bD,P13),In(bD,P15),In(bC,P6)};


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
	_GPIO_WriteBit(pin.bank, pin.pin, value);
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
	_GPIO_WriteBit(pin.bank, pin.pin, value);
}

inline uint8_t getInDirect(IOPin pin)
{
	if(pin.bank==bU)
		return 0;
	return _GPIO_ReadInputDataBit(pin.bank, pin.pin);
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
	return _GPIO_ReadInputDataBit(pin.bank, pin.pin);
}

void initIOs()
{
	for(int i=0;i<4;i++)
			mInputState[i]=0;
	for(int i=0;i<nHeldRelay;i++)
	{
		heldRelayState[i]=0;
		lastHeldRelayOnTime[i]=0;
		initOutput(heldRelays[i].pin);
	}

	for(int i=0;i<nLED;i++)
	{
		ledState[i].state=0;
		ledState[i].pwm=64;
		ledState[i].pwmFunc=NULL;
		ledState[i].data=NULL;
	}
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

uint32_t ioTicks=0;

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
		uint8_t id8=i/8;
		uint8_t im8s=1<<(i%8);
		uint8_t oldState=mLEDState[id8]&im8s;
		uint8_t newState=oldState;
		switch(ledState[i].state)
		{
		case OFF:
			newState=0;
			break;
		case ON:
			newState=1;
			break;
		case FLASHING:
			newState=(ledState[i].flashAt--)<ledState[i].flashMax;
			if(ledState[i].flashAt<=0)
				ledState[i].flashAt=ledState[i].flashMax*2;
			break;
		case PWM:
		{
			uint8_t pwm;;
			if(ledState[i].pwmFunc)
				pwm=ledState[i].pwmFunc(ledState[i].data);
			else
				pwm=ledState[i].pwm;
			newState=(ioTicks&0xFF)<pwm;
			break;
		}
		}
		if(!newState && oldState)//should be off
		{
			mLEDState[id8]&= ~im8s;
			LED_Dirty=1;
		}
		else if(newState && !oldState)
		{
			mLEDState[id8]|= im8s;
			LED_Dirty=1;
		}
	}
	if(LED_Dirty)
	{
		//STM_EVAL_LEDToggle(LED4);
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
	for(int i=0;i<nHeldRelay;i++)
	{
		if(lastHeldRelayOnTime[i]+heldRelayMaxOnTime[i]>msElapsed)
		{
			setHeldRelay(i,0);
		}
	}
	ioTicks++;
}

void setLedDebug(uint8_t n, enum LEDs leds[8])
{
	for(int i=0;i<8;i++)
	{
		if(n&1)
		{
			setLed(leds[i],ON);
		}
		else
		{
			setLed(leds[i],OFF);
		}
		n>>=1;
	}
}

void updateSlowInputs()
{
	setOutDirect(MULTI_IN_LATCH,0);
	uint8_t in[nMultiInput];
	for(int i=0;i<nMultiInput;i++)
		in[i]=0;
	setLedDebug(0,{RED_TARGET_LEFT,LEFT_1,LEFT_2,LEFT_3,RIGHT_1,RIGHT_2,RIGHT_3,RED_TARGET_RIGHT});
	setLedDebug(0,{TOP_3,RED_TARGET_TOP,FIVE_1,FIVE_2,FIVE_3,FIVE_4,FIVE_5,RED_TARGET_BOTTOM});
	for(int i=0;i<8;i++)
	{
		setOutDirect(MULTI_IN_CLOCK,0);
		for(int j=0;j<nMultiInput;j++)
		{
			in[j]<<=1;
			in[j]|=getInDirect(MULTI_IN_DATA[j]);
			if(getInDirect(MULTI_IN_DATA[j]))
			{
				setLedDebug(i,{RED_TARGET_LEFT,LEFT_1,LEFT_2,LEFT_3,RIGHT_1,RIGHT_2,RIGHT_3,RED_TARGET_RIGHT});
				setLedDebug(j,{TOP_3,RED_TARGET_TOP,FIVE_1,FIVE_2,FIVE_3,FIVE_4,FIVE_5,RED_TARGET_BOTTOM});
			}
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

uint8_t fireSolenoid(Solenoid *s)
{
	return fireSolenoidFor(s, s->onTime);
}

uint8_t fireSolenoidFor(Solenoid *s, uint32_t ms)
{
	if(s->offTime+s->lastFired>msElapsed)
		return 0;
	//while(msElapsed<lastSolenoidFiringTime+50);
	setOut(s->pin, 1);
	callFuncIn_s(turnOffSolenoid, ms,s);
	return 1;
}

void setLed(enum LEDs index,uint8_t state)
{
	ledState[index].state=state;
}

void setPWM(enum LEDs index, uint8_t pwm)
{
	ledState[index].state=PWM;
	ledState[index].pwm=pwm;
	ledState[index].pwmFunc=NULL;
}

void setPWMFunc(enum LEDs index, uint8_t (*pwmFunc)(void*), void *data)
{
	ledState[index].state=PWM;
	ledState[index].pwmFunc=pwmFunc;
	ledState[index].data=data;
}

void offsetLed(enum LEDs index,uint32_t offset)
{
	ledState[index].flashAt+=offset;
}
void setFlash(enum LEDs index, uint32_t max)
{
	ledState[index].state=FLASHING;
	ledState[index].flashMax=max;
	ledState[index].flashAt=0;
}
uint8_t getLed(enum LEDs index)
{
	if(ledState[index].state==0)
		return OFF;
	if(ledState[index].state==4294967295)
		return ON;
	if(ledState[index].state==1)
		return PWM;
	return FLASHING;
}

void setHeldRelay(int n,uint8_t state)
{
	heldRelayState[n]=state;
	if(state)
	{
		fireSolenoidFor(&heldRelays[n],20);
		lastHeldRelayOnTime[n]=msElapsed;
	}
	else
	{
		fireSolenoidFor(&HOLD,20);
		for(int i=0;i<nHeldRelay;i++)
			if(heldRelayState[i])
				fireSolenoidFor(&heldRelays[i],20);
	}
}
