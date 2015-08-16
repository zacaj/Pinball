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
#include "game.h"
#include <string.h>
#include <stdlib.h>
void setLedDebug(uint8_t n, enum LEDs leds[8]);

#define _GPIO_WriteBit(b,p,v) \
	if(!v) ((b))->BRR = ((p)); \
else ((b))->BSRR = ((p));
#define _writeHigh(b) \
		((b)).bank->BSRR = ((b)).pin
#define _writeLow(b) \
		((b)).bank->BRR = ((b)).pin

#define _GPIO_ReadInputDataBit(b,p) \
	(( ( ((b))->IDR & ((p)) ) != 0 ))

uint8_t mInputState[nMultiInput];
typedef struct {
	uint8_t state;
	int32_t flashAt, flashMax;
	void *data;
	uint8_t pwm;
	uint8_t (*pwmFunc)(void*);
} LedState;
uint8_t mLEDState[6];
uint8_t LED_Dirty = 0;
uint8_t heldRelayState[nHeldRelay];
uint8_t physicalHeldRelayState[nHeldRelay];
uint32_t lastHeldRelayOnTime[nHeldRelay];
LedState ledState[nLED];

Solenoid HOLD = Sd(bF,P4,20);
Solenoid SCORE[4] = { Sds(bB,P4,90,150), Sds(bD,P5,90,150), Sds(bC,P10,90,150), Sds(bB,P8,90,150) };
Solenoid BONUS[4] = { Sds(bB,P5,90,150), Sds(bB,P9,90,150), Sds(bC,P13,90,150), Sds(bF,P10,90,150) };
Solenoid BALL_SHOOT = Sds(bD,P6,220,500);
Solenoid BALL_ACK = Sds(bA,P15,300,800);
Solenoid LEFT_DROP_RESET = Sds(bC,P8,100,700);
Solenoid RIGHT_DROP_RESET = Sds(bF,P6,100,700);
Solenoid TOP_DROP_RESET = Sds(bC,P3,100,700);
Solenoid FIVE_DROP_RESET = Sds(bC,P1,175,700);
Solenoid LEFT_CAPTURE_EJECT = Sds(bC,P5,120,700);
Solenoid RIGHT_CAPTURE_EJECT = Sds(bA,P8,120,700);
Solenoid TOP_CAPTURE_EJECT = Sds(bA,P1,120,700);
Solenoid heldRelays[nHeldRelay] = {
	/*Player enable 1*/Sd(bD,P3,20),
	Sd(bC,P12,20),
	Sd(bD,P7,20),
	Sd(bD,P1,20),/*Player enable 4*/
	Sd(bD,P4,-1), //ball shoot enable
	Sd(bD,P2,-1), //ball_release
	Sd(bA,P10,-1), //magnet
	Sd(bD,P0,-1), //left block
	Sd(bC,P11,-1), //right block
	Sd(bA,P3,20), //playfield disable
};
//left right top
Input DROP_TARGET[3][3] = { { In(bmA,P0), In(bmA,P3), In(bmA,P2) }, { In(bmA,P4),
In(bmA, P7), In(bmA,P6) }, { In(bmB,P1), In(bmB,P2), In(bmB,P0) } };
Input FIVE_TARGET[5] = { In(bmB,P3), In(bmB,P4), In(bmB,P7), In(bmB,P6), In(bmB,P5) };
Input LEFT_CAPTURE = In(bB,P14);
Input RIGHT_CAPTURE = In(bmA,P5);
Input TOP_CAPTURE = In(bmA,P1);
Input SCORE_ZERO[4] = { In(bmD,P0), In(bmD,P1), In(bmD,P2), In(bmD,P4) };
Input BONUS_ZERO[4] = { In(bmD,P5), In(bmD,P6), In(bmD,P7), In(bmD,P3) };
Input BALL_OUT = In(bB,P10);
Input SHOOT_BUTTON = In(bmC,P3);
Input BALLS_FULL = In(bB,P12);
Input LANES[4] = { In(bD,P8), In(bD,P10), In(bD,P12), In(bD,P14) };
Input LEFT_FLIPPER = In(bmC,P6);
Input LEFT_BLOCK = In(bmC,P7);
Input RIGHT_FLIPPER = In(bmC,P4);
Input RIGHT_BLOCK = In(bmC,P5);
Input START = In(bmC,P1);
Input CAB_LEFT = In(bmC,P0);
Input CAB_RIGHT = In(bmC,P2);
Input LEFT_POP = In(bB,P11);
Input RIGHT_POP = In(bB,P13);
Input BUMPER = In(bC,P7);
Input ROTATE_ROLLOVER = In(bB,P15);
Input ACTIVATE_TARGET = In(bD,P9);
Input RED_TARGET[4] = { In(bD,P13), In(bC,P6), In(bD,P15), In(bD,P11) };

Input COM_ACK = In(bE,P7);
IOPin COM_DATA = {bE,P6};
IOPin COM_CLOCK = {bF,P9};

#define MAX_COMMANDS 8
uint8_t commandQueue[MAX_COMMANDS];
uint8_t commandAt=0;
uint8_t nCommand=0;
uint32_t lastAckTime=0;
uint32_t commandStartSendTime=0;
uint8_t comClock=1;

void initInput(IOPin pin, GPIOPuPd_TypeDef def) {
	if (pin.bank == bU)
		return;
	if ((int) pin.bank < 11)
		return;
	GPIO_InitTypeDef init;
	init.GPIO_Mode = GPIO_Mode_IN;
	init.GPIO_OType = GPIO_OType_PP;
	init.GPIO_Pin = pin.pin;
	init.GPIO_PuPd = PULL_DOWN;

	GPIO_Init(pin.bank, &init);
}

void initOutput(IOPin pin) {
	if (pin.bank == bU)
		return;
	if ((int) pin.bank < 11)
		return;
	GPIO_InitTypeDef init;
	init.GPIO_Mode = GPIO_Mode_OUT;
	init.GPIO_OType = GPIO_OType_PP;
	init.GPIO_Pin = pin.pin;
	init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(pin.bank, &init);
}

inline void setOutDirect(IOPin pin, uint32_t value) {
	if (pin.bank == bU)
		return;
	_GPIO_WriteBit(pin.bank, pin.pin, value);
}

void setOut(IOPin pin, uint32_t value) {
	if (pin.bank == bU)
		return;
	if ((int) pin.bank < 11) {
		if ((int) pin.bank >= 5 && (int) pin.bank <= 10) {
			if ((mLEDState[(int) pin.bank - 5] & 1 << pin.pin) ?
					1 : 0 != value) {
				LED_Dirty = 1;
				if (value)
					mLEDState[(int) pin.bank - 5] |= 1 << pin.pin;
				else
					mLEDState[(int) pin.bank - 5] &= ~(1 << pin.pin);
			}
		}
		return;
	}
	_GPIO_WriteBit(pin.bank, pin.pin, value);
}

uint8_t getInDirect(IOPin pin) {
	if (pin.bank == bU)
		return 0;
	return _GPIO_ReadInputDataBit(pin.bank, pin.pin);
}

inline uint8_t getIn(IOPin pin) {
	//if (pin.bank == bU)
		//return 0;
	if ((int) pin.bank < 11) {
		if ((int) pin.bank >= 1 && (int) pin.bank <= 4)
			return (mInputState[(int) pin.bank - 1] & (pin.pin)) ? 1 : 0;
		return 0;
	}
	return _GPIO_ReadInputDataBit(pin.bank, pin.pin);
}

void initIOs() {
	for (int i = 0; i < 4; i++)
		mInputState[i] = 0;

	for (int i = 0; i < nLED; i++) {
		ledState[i].state = 0;
		ledState[i].pwm = 64;
		ledState[i].pwmFunc = NULL;
		ledState[i].data = NULL;
	}
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOE, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOF, ENABLE);

	initOutput(HOLD.pin);
	initOutput(BALL_SHOOT.pin);
	initOutput(BALL_ACK.pin);
	initOutput(LEFT_DROP_RESET.pin);
	initOutput(RIGHT_DROP_RESET.pin);
	initOutput(TOP_DROP_RESET.pin);
	initOutput(FIVE_DROP_RESET.pin);
	initOutput(LEFT_CAPTURE_EJECT.pin);
	initOutput(RIGHT_CAPTURE_EJECT.pin);
	initOutput(TOP_CAPTURE_EJECT.pin);
	initOutput(MULTI_IN_LATCH);
	initOutput(MULTI_IN_CLOCK);
	setOutDirect(MULTI_IN_CLOCK, 0);
	setOutDirect(MULTI_IN_LATCH, 1);
	initOutput(LED_LATCH);
	setOutDirect(LED_LATCH, 0);
	initOutput(LED_DATA);
	setOutDirect(LED_DATA, 0);
	initOutput(LED_CLOCK);
	setOutDirect(LED_CLOCK, 0);
	initOutput(COM_DATA);
	setOutDirect(COM_DATA,0);
	initOutput(COM_CLOCK);
	setOutDirect(COM_CLOCK,1);
	comClock=1;
	//initInput(COM_ACK.pin, PULL_UP);
	GPIO_InitTypeDef init;
		init.GPIO_Mode = GPIO_Mode_IN;
		init.GPIO_OType = GPIO_OType_PP;
		init.GPIO_Pin = COM_ACK.pin.pin;
		init.GPIO_PuPd = PULL_UP;

		GPIO_Init(COM_ACK.pin.bank, &init);
	//COM_ACK.inverse=1;
	//COM_ACK.inverse=1;

	for (int i = 0; i < nMultiInput; i++) {
		initInput(MULTI_IN_DATA[i], PULL_DOWN);
	}
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++) {
			initInput(DROP_TARGET[i][j].pin, NO_PULL);
			DROP_TARGET[i][j].settleTime=50;
		}
	for (int i = 0; i < 4; i++) {
		initOutput(SCORE[i].pin);
		initOutput(BONUS[i].pin);
		initInput(SCORE_ZERO[i].pin, NO_PULL);
		SCORE_ZERO[i].settleTime=15;
		initInput(BONUS_ZERO[i].pin, NO_PULL);
		BONUS_ZERO[i].settleTime=15;
		initInput(LANES[i].pin, PULL_DOWN);
		LANES[i].settleTime=0;
		initInput(RED_TARGET[i].pin, PULL_DOWN);
	}
	LANES[3].settleTime=0;
	for (int i = 0; i < 5; i++)
		initInput(FIVE_TARGET[i].pin, NO_PULL);

	initInput(LEFT_CAPTURE.pin, NO_PULL);
	LEFT_CAPTURE.settleTime=50;
	initInput(RIGHT_CAPTURE.pin, NO_PULL);
	RIGHT_CAPTURE.settleTime=50;
	initInput(TOP_CAPTURE.pin, NO_PULL);
	TOP_CAPTURE.settleTime=50;
	initInput(BALL_OUT.pin, NO_PULL);
	BALL_OUT.settleTime=200;
	initInput(SHOOT_BUTTON.pin, NO_PULL);
	initInput(START.pin, NO_PULL);
	initInput(CAB_LEFT.pin, NO_PULL);
	initInput(CAB_RIGHT.pin, NO_PULL);
	initInput(LEFT_FLIPPER.pin, PULL_DOWN);
	initInput(RIGHT_FLIPPER.pin, PULL_DOWN);
	initInput(LEFT_BLOCK.pin, PULL_DOWN);
	initInput(RIGHT_BLOCK.pin, PULL_DOWN);
	initInput(ACTIVATE_TARGET.pin, PULL_DOWN);
	initInput(LEFT_POP.pin, PULL_DOWN);
	initInput(RIGHT_POP.pin, PULL_DOWN);
	initInput(BUMPER.pin, PULL_DOWN);
	BUMPER.inverse=1;
	initInput(ROTATE_ROLLOVER.pin, PULL_DOWN);
	initInput(BALLS_FULL.pin, PULL_DOWN);
	BALLS_FULL.settleTime=70;
	
	for (int i = 0; i < nHeldRelay; i++) {
		physicalHeldRelayState[i] = 0;
		heldRelayState[i] = 0;
		lastHeldRelayOnTime[i] = 0;
		initOutput(heldRelays[i].pin);
	}

	fireSolenoidFor(&HOLD,50);
}

uint16_t getLedOverride(enum LEDs led);
int nled=0;

__attribute__((optimize("O3"))) void updateInput(Input* in, uint8_t state) {
	if(in->inverse)
		state=!state;
	in->pressed = 0;
	in->released = 0;
	if(state != in->rawState) {
		in->rawState=state;
		if(in->rawState) {
			in->lastRawOff=in->lastChange;
			in->lastRawOn=msElapsed;
		}
		else {
			in->lastRawOn=in->lastChange;
			in->lastRawOff=msElapsed;
		}
		in->lastRawChange=msElapsed;
	}
	if (in->rawState != in->state && (msElapsed - in->lastRawChange >= in->settleTime)) {
		in->state = in->rawState;
		if(in->state) {
			in->lastOff=in->lastChange;
			in->pressed = 1;
			nled++;
		}
		else {
			in->lastOn=in->lastChange;
			in->released = 1;
		}
		in->lastChange = msElapsed;
	}
}

#define updateDirectInput(in) \
	updateInput((in), _GPIO_ReadInputDataBit((in)->pin.bank, (in)->pin.pin))
	
#define updateSlowInput(in) \
	updateInput((in), (mInputState[(int) (in)->pin.bank - 1] & ((in)->pin.pin))?1:0)

void updateCommands();

__attribute__((optimize("O0"))) static void updateLeds() {
	for (int j = 0; j < 6; j++)
		for (int i = 0; i < 8; i++) {
			_writeLow(LED_CLOCK);
			if(mLEDState[j] & (1 << i))
				_writeHigh(LED_DATA);
			else
				_writeLow(LED_DATA);
			_writeHigh(LED_CLOCK);
			asm("nop;nop;");
			asm("nop;nop;");
			asm("nop;nop;");
		}
	_writeLow(LED_DATA);
	_writeHigh(LED_LATCH);
	setOutDirect(LED_LATCH, 0);
	LED_Dirty = 0;
}

uint32_t ioTicks = 0;
uint32_t ioTicksAt = 0;

void updateIOs() {
	if(ioTicks%1==0)
	{
		updateSlowInputs();
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				updateSlowInput(&DROP_TARGET[i][j]);
		for (int i = 0; i < 5; i++)
			updateSlowInput(&FIVE_TARGET[i]);
		for (int i = 0; i < 4; i++) {
			updateSlowInput(&SCORE_ZERO[i]);
			updateSlowInput(&BONUS_ZERO[i]);
			updateDirectInput(&LANES[i]);
			updateDirectInput(&RED_TARGET[i]);
		}
		updateDirectInput(&LEFT_CAPTURE);//30
		updateSlowInput(&RIGHT_CAPTURE);//
		updateSlowInput(&TOP_CAPTURE);
		updateDirectInput(&BALLS_FULL);//
		updateDirectInput(&BALL_OUT);
		updateSlowInput(&SHOOT_BUTTON);//
		updateSlowInput(&START);//35
		updateSlowInput(&CAB_LEFT);
		updateSlowInput(&CAB_RIGHT);
		updateSlowInput(&LEFT_FLIPPER);
		updateSlowInput(&RIGHT_FLIPPER);//
		updateSlowInput(&LEFT_BLOCK);//40
		updateSlowInput(&RIGHT_BLOCK);//
		updateDirectInput(&ACTIVATE_TARGET);
		updateDirectInput(&LEFT_POP);
		updateDirectInput(&RIGHT_POP);//
		updateDirectInput(&BUMPER);//45
		updateDirectInput(&ROTATE_ROLLOVER);//
		updateDirectInput(&COM_ACK);
	}

	for (int i = 0; i < nLED; i++) {
		uint8_t id8 = i / 8;
		uint8_t im8s = 1 << (i % 8);
		uint8_t oldState = mLEDState[id8] & im8s;
		uint8_t newState = oldState;
		if(mode == PLAYER_SELECT && 0) {
			uint16_t n = getLedOverride(i);
			if(n!=256) {
				newState = (ioTicks & 0xFF) < n;//n>128;//
				goto done;
			}
		}
		switch (ledState[i].state) {
		case OFF:
			newState = 0;
			break;
		case ON:
			newState = 1;
			break;
		case FLASHING:
			newState = (ledState[i].flashAt--) < ledState[i].flashMax;
			if (ledState[i].flashAt <= 0)
				ledState[i].flashAt = ledState[i].flashMax * 2;
			break;
		case PWM: {
			uint8_t pwm;
			;
			if (ledState[i].pwmFunc)
				pwm = ledState[i].pwmFunc(ledState[i].data);
			else
				pwm = ledState[i].pwm;
			newState = (ioTicks & 0xFF) < pwm;
			break;
			}
		}
	done:
		if (!newState && oldState) //should be off
		{
			mLEDState[id8] &= ~im8s;
			LED_Dirty = 1;
		} else if (newState && !oldState) {
			mLEDState[id8] |= im8s;
			LED_Dirty = 1;
		}
	}
	if (LED_Dirty) {
		updateLeds();
	}
	if(ioTicks%4==0)
		for (int i = 0; i < nHeldRelay; i++) {
			uint32_t offAt=lastHeldRelayOnTime[i] + heldRelayMaxOnTime[i];
			if (heldRelayState[i] && offAt > msElapsed && lastHeldRelayOnTime[i]!=-1) {
				setHeldRelay(i, 0);
			}
		}
	
	if(nCommand>0) {
		int oldNCommand = nCommand;
		uint32_t start = msElapsed;
		while(oldNCommand == nCommand && msElapsed - start < 1)
			updateCommands();
	}

	/*if(msElapsed!=ioTicksAt) {
		ioTicksAt=msElapsed;
		ioTicks=0;
	}*/
	ioTicks++;
}

void setLedDebug(uint8_t n, enum LEDs leds[8]) {
	for (int i = 0; i < 8; i++) {
		if (n & 1) {
			setLed(leds[7-i], ON);
		} else {
			setLed(leds[7-i], OFF);
		}
		n >>= 1;
	}
}

void setBoardLed(int i, uint8_t state)
{
	if(state)

		STM_EVAL_LEDOn(LED3+i);else
			STM_EVAL_LEDOff(LED3+i);
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
			in[j]>>=1;
			uint8_t state=getInDirect(MULTI_IN_DATA[j]);
			in[j]|=state<<7;
		}
		setOutDirect(MULTI_IN_CLOCK,1);
	}
	for(int i=0;i<nMultiInput;i++) {
		mInputState[i]=in[i];
	}
	if(in[3]==0) {
		_BREAK();
		_BREAK();
	}
	setOutDirect(MULTI_IN_LATCH,1);
	setBoardLed(0,SCORE_ZERO[0].state);//3
	setBoardLed(1,SCORE_ZERO[1].state);
	setBoardLed(2,SCORE_ZERO[2].state);
	setBoardLed(3,SCORE_ZERO[3].state);//6
	setBoardLed(4,BONUS_ZERO[0].state);
	setBoardLed(5,BONUS_ZERO[1].state);
	setBoardLed(6,BONUS_ZERO[2].state);//9
	setBoardLed(7,BONUS_ZERO[3].state);/**/
}

//uint32_t lastSolenoidFiringTime= 0;

uint32_t turnOffSolenoid(Solenoid *s) {
	setOut(s->pin, 0);
	s->state=0;
	//STM_EVAL_LEDToggle(LED3);
	//free(pin);
	s->lastFired = msElapsed;
	return 1;
}

uint32_t fireSolenoidAfter(Solenoid *s) {
	fireSolenoid(s);
	s->waitingToFire=0;
	return 1;
}

uint8_t fireSolenoid(Solenoid *s) {
	return fireSolenoidFor(s, s->onTime);
}

uint8_t fireSolenoidForAnd(Solenoid *s, uint32_t ms, uint8_t force) {
	if (s->offTime + s->lastFired > msElapsed && !force && s->lastFired<msElapsed)
		return 0;
	//while(msElapsed<lastSolenoidFiringTime+50);
	setOut(s->pin, 1);
	s->state=1;
	s->lastLastFired = s->lastFired;
	s->lastFired = msElapsed;
	//STM_EVAL_LEDToggle(LED3);
	callFuncIn_s(turnOffSolenoid, ms, s);
	return 1;
}

void fireSolenoidAlways(Solenoid *s) {
	fireSolenoidForAnd(s, s->onTime, 1);
}

uint8_t fireSolenoidFor(Solenoid *s, uint32_t ms) {
	return fireSolenoidForAnd(s,ms,0);
}
void fireSolenoidIn(Solenoid *s, uint32_t ms) {
	if(s->waitingToFire)
		return;
	s->waitingToFire=1;
	callFuncIn(fireSolenoidAfter,ms,s);
}


uint8_t setLed(enum LEDs index, uint8_t state) {
	if(ledState[index].state==state)
		return 0;
	ledState[index].state = state;
	return 1;
}

void setPWM(enum LEDs index, uint8_t pwm) {
	ledState[index].state = PWM;
	ledState[index].pwm = pwm;
	ledState[index].pwmFunc = NULL;
}

void setPWMFunc(enum LEDs index, uint8_t (*pwmFunc)(void*), void *data) {
	ledState[index].state = PWM;
	ledState[index].pwmFunc = pwmFunc;
	ledState[index].data = data;
}

void offsetLed(enum LEDs index, uint32_t offset) {
	ledState[index].flashAt += offset;
}
uint8_t setFlash(enum LEDs index, uint32_t max) {
	if(ledState[index].state==FLASHING && ledState[index].flashMax==max)
		return 0;
	ledState[index].state = FLASHING;
	ledState[index].flashMax = max;
	ledState[index].flashAt = 0;
	return 1;
}
void syncLeds(enum LEDs a, enum LEDs b, enum LEDs c, enum LEDs d) {
	int flashAvg = ledState[a].flashAt+ledState[b].flashAt;
	int size=2;
	if(c!=nLED) {
		flashAvg+=ledState[c].flashAt;
		size++;
	}
	if(d!=nLED) {
		flashAvg+=ledState[d].flashAt;
		size++;
	}
	flashAvg/=size;
	ledState[a].flashAt=flashAvg;
	ledState[b].flashAt=flashAvg;
	if(size>=3)
		ledState[c].flashAt=flashAvg;
	if(size>=4)
		ledState[d].flashAt=flashAvg;
}
uint8_t getLed(enum LEDs index) {
	return ledState[index].state;
}

void updateHeldRelays()
{
	uint8_t releaseHold=0;

	uint32_t start=msElapsed;
	uint8_t on=0;
	for(int i=0;i<nHeldRelay;i++)
		if(heldRelayState[i]) {
			lastHeldRelayOnTime[i]=msElapsed;
			setOut(heldRelays[i].pin,1);
			heldRelays[i].state=1;
			on=1;
		}
		else {	
			setOut(heldRelays[i].pin,0);
			heldRelays[i].state=0;
			if(physicalHeldRelayState[i])
				releaseHold=1;
		}
	if(releaseHold)
	{
		fireSolenoidForAnd(&HOLD, 80,1);
		wait(150);
	}

	while(start+20>msElapsed);
	if(on)
		for(int i=0;i<nHeldRelay;i++)
			if(heldRelayState[i]) {
				setOut(heldRelays[i].pin,0);			
				heldRelays[i].state=0;
			}

	for (int i = 0; i < nHeldRelay; i++)
		physicalHeldRelayState[i]=heldRelayState[i];
}

void setHeldRelay(int n, uint8_t state) {
	if (heldRelays[n].onTime == -1) {
		lastHeldRelayOnTime[n] = msElapsed;
		setOut(heldRelays[n].pin, state);
		heldRelays[n].state=state;
	}
	else if(physicalHeldRelayState[n]!=state || 1) {
		heldRelayState[n] = state;
		_BREAK();
		updateHeldRelays();
	}
}

uint32_t lastCommandSendTime=0;

uint8_t sendCommand(uint8_t cmd) {
	uint8_t ret = 0;
	
	lastCommandSendTime=msElapsed;
	if(nCommand>=MAX_COMMANDS) {
		uint8_t least=255;
		int leastAt=-1;
		for(int i=1;i<nCommand;i++) {
			if(commandQueue[i]<least) {
				least=commandQueue[i];
				leastAt=i;
			}
		}
		if(least>cmd && leastAt>=0) {
			commandQueue[leastAt]=cmd;
			ret = 1;
		}
	}
	else {
		commandQueue[nCommand++]=cmd;
		ret = 2;
	}
	
	if(ret) {
		int oldNCommand = nCommand;
		uint32_t start = msElapsed;
		while(oldNCommand == nCommand && msElapsed - start < 10) {
			updateCommands();
			updateDirectInput(&COM_ACK);
		}
	}
	
	return ret;
}
uint8_t sendCommandElse(uint8_t cmd) {
	if(msElapsed-lastCommandSendTime<1 && lastCommandSendTime!=0)
		return 0;
	return sendCommand(cmd);
}
uint32_t nCommandsSent=0;
uint32_t nCommandRollingAverage=0;
void updateCommands() {
	if(!COM_ACK.rawState) {
		if(comClock) {
			lastAckTime=msElapsed;
			setOutDirect(COM_CLOCK,0);
			comClock=0;
			setOutDirect(COM_DATA,(commandQueue[0]&(1<<commandAt))? 1:0);
			if(commandAt==0)
				commandStartSendTime=msElapsed;
			commandAt++;
		}
		else {
			
		}
	}
	else {
		if(comClock) {
			if(msElapsed-lastAckTime>100 && lastAckTime!=0) {
				commandAt=0;
			}
		}
		else {
			setOutDirect(COM_CLOCK,1);
			comClock=1;
			if(commandAt==sizeof(commandQueue[0])*8) {
				commandAt=0;
				for(int i=1;i<nCommand;i++) {
					commandQueue[i-1]=commandQueue[i];
				}
				nCommand--;
				nCommandsSent++;
				nCommandRollingAverage+=msElapsed-commandStartSendTime;
				commandStartSendTime=0;
			}
		}
	}
}

typedef struct {
	int x,y;
} LightPos;

LightPos lightPos[] = {
	{12655, 25833}, //TOP_1
	{12333, 26481}, //TOP_2
	{14910, 27128}, //TOP_3
	{16198, 30100}, //TOP_CAPTURE_LIGHT
	{17083, 26549}, //RED_TARGET_TOP
	{18050, 24812}, //FIVE_2
	{22880, 21985}, //RED_TARGET_BOTTOM
	{-1, -1}, //BALL_1
	{16955, 25528}, //FIVE_1
	{20948, 22670}, //FIVE_5
	{18935, 24080}, //FIVE_3
	{19982, 23356}, //FIVE_4
	{21833, 28691}, //RIGHT_POP_LEFT
	{6585, 27128}, //LEFT_CAPTURE_LIGHT
	{27952, 28538}, //RIGHT_POP_RIGHT
	{26664, 25757}, //RIGHT_POP_BOTTOM
	{13058, 15423}, //LEFT_3
	{24893, 15088}, //RED_TARGET_RIGHT
	{7922, 15088}, //RED_TARGET_LEFT
	{21511, 15164}, //RIGHT_2
	{13461, 6210}, //LOCK_1
	{16359, 5639}, //SHOOT_AGAIN
	{17486, 6858}, //LOCK_3
	{15071, 6858}, //LOCK_2
	{28757, 10973}, //LANE_3
	{29965, 13526}, //RIGHT_CAPTURE_LIGHT
	{31172, 10973}, //LANE_4
	{19499, 6210}, //LOCK_4
	{19740, 15431}, //RIGHT_3
	{9596, 14859}, //LEFT_1
	{23122, 14859}, //RIGHT_1
	{11367, 15164}, //LEFT_2
	{-1, -1}, //BALL_2
	{-1, -1}, //BALL_3
	{-1, -1}, //BALL_4
	{-1, -1}, //BALL_5
	{4122, 9944}, //RED_LED
	{6376, 12268}, //WHITE_LED
	{6376, 13412}, //PURPLE_LED
	{8952, 18898}, //LEFT_POP_LIGHT
	{-1, -1}, //HOLD_1
	{3800, 10973}, //LANE_2
	{1787, 10973}, //LANE_1
	{4122, 13412}, //YELLOW_LED
	{-1, -1}, //HOLD_2
	{-1, -1}, //HOLD_3
	{-1, -1}, //HOLD_4
	{-1, -1}, //TEMP_BALL_LAUNCH_READY
};

uint16_t getLedOverride(enum LEDs led) {
	int t = (msElapsed%3000)*32768/3000; //0-1000
	LightPos pos = lightPos[led];
	int l = abs((pos.y*1000) - (t*1000));
	int percent = 10;
	int cutoff = (((percent * 1000) / 100) * 32768);
	if(l > cutoff)
		return 0;
	return 255- l * 255 / cutoff;
}
