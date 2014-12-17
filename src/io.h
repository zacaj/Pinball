/*
 * io.h
 *
 *  Created on: Jul 16, 2014
 *      Author: zacaj
 */

#ifndef IO_H_
#define IO_H_
#include "stm32f30x.h"
#include "stm32f3_discovery.h"
#include "stm32f30x_conf.h"
#include <stdio.h>
#include <stdint.h>

typedef struct
{
	GPIO_TypeDef* bank;
	uint32_t pin;
} IOPin;
#define NO_PULL GPIO_PuPd_NOPULL
#define PULL_DOWN GPIO_PuPd_DOWN
#define PULL_UP GPIO_PuPd_UP
#define bA GPIOA
#define bB GPIOB
#define bC GPIOC
#define bD GPIOD
#define bE GPIOE
#define bF GPIOF
#define bmA (GPIO_TypeDef*)1
#define bmB (GPIO_TypeDef*)2
#define bmC (GPIO_TypeDef*)3
#define bmD (GPIO_TypeDef*)4
#define bU (GPIO_TypeDef*)100
#define P0 GPIO_Pin_0
#define P1 GPIO_Pin_1
#define P2 GPIO_Pin_2
#define P3 GPIO_Pin_3
#define P4 GPIO_Pin_4
#define P5 GPIO_Pin_5
#define P6 GPIO_Pin_6
#define P7 GPIO_Pin_7
#define P8 GPIO_Pin_8
#define P9 GPIO_Pin_9
#define P10 GPIO_Pin_10
#define P11 GPIO_Pin_11
#define P12 GPIO_Pin_12
#define P13 GPIO_Pin_13
#define P14 GPIO_Pin_14
#define P15 GPIO_Pin_15

#define nMultiInput 4
static const IOPin MULTI_IN_LATCH= {bA,P6};
static const IOPin MULTI_IN_CLOCK= {bC,P4};
static const IOPin MULTI_IN_DATA[nMultiInput]=  { {bB,P2},{bB,P0},{bA,P4},{bA,P2} };
static const IOPin LED_LATCH={bC,P0};
static const IOPin LED_DATA={bC,P2};
static const IOPin LED_CLOCK={bF,P2};


typedef struct
{
	IOPin pin;
	uint32_t lastFired;
	uint32_t onTime,offTime;
} Solenoid;
#define S(b,p) {{b,p},0,90,250}
#define Sd(b,p,on) {{b,p},0,on,250}
#define Sds(b,p,on,off) {{b,p},0,on,off}

extern Solenoid HOLD;
static const int PLAYFIELD_DISABLE=9;
static const int PLAYER_ENABLE[]={0,1,2,3};
extern Solenoid SCORE[4];
extern Solenoid BONUS[4];

static const int BALL_SHOOT_ENABLE=4;
static const int BALL_RELEASE=5;
extern Solenoid BALL_SHOOT;
extern Solenoid BALL_ACK;

extern Solenoid LEFT_DROP_RESET,RIGHT_DROP_RESET,TOP_DROP_RESET;
extern Solenoid FIVE_DROP_RESET;
static const int MAGNET=6;
static const int LEFT_BLOCK_DISABLE=7;
static const int RIGHT_BLOCK_DISABLE=8;
extern Solenoid LEFT_CAPTURE_EJECT,RIGHT_CAPTURE_EJECT,TOP_CAPTURE_EJECT;

#define nHeldRelay 10
extern Solenoid heldRelays[nHeldRelay];
extern uint32_t lastHeldRelayOnTime[nHeldRelay];
static const uint32_t heldRelayMaxOnTime[nHeldRelay]={
		-1,-1,-1,-1,
		-1,
		3000,
		8000,
		-1,-1,-1

};

typedef struct
{
	IOPin pin;
	uint8_t state;
	uint32_t lastChange;
	uint8_t pressed;
	uint8_t released;
} Input;
#define In(b,p) {{b,p},0,1,0,0}

extern Input DROP_TARGET[3][3];
extern Input FIVE_TARGET[5];
extern Input LEFT_CAPTURE;
extern Input RIGHT_CAPTURE;
extern Input TOP_CAPTURE;
extern Input SCORE_ZERO[4];
extern Input BONUS_ZERO[4];
extern Input BALL_OUT;
extern Input BALL_LOADED;
extern Input BALLS_FULL;
extern Input LANES[4];
extern Input LEFT_FLIPPER;
extern Input LEFT_BLOCK;
extern Input RIGHT_FLIPPER;
extern Input RIGHT_BLOCK;
extern Input START;
extern Input CAB_LEFT;
extern Input CAB_RIGHT;
extern Input LEFT_POP;
extern Input RIGHT_POP;
extern Input BUMPER;
extern Input ROTATE_ROLLOVER;
extern Input ACTIVATE_TARGET;
extern Input RED_TARGET[4];

extern uint8_t heldRelayState[nHeldRelay];
void initInput(IOPin pin, GPIOPuPd_TypeDef def);
void initOutput(IOPin pin);
void setOut(IOPin pin, uint32_t value);
void setOutDirect(IOPin pin, uint32_t value);
uint8_t getIn(IOPin pin);
void initIOs();
void updateIOs();

uint8_t fireSolenoidFor(Solenoid *s, uint32_t ms);
uint8_t fireSolenoid(Solenoid *s);


#define OFF 0
#define ON 1
#define FLASHING 2
#define PWM 3
#define nLED 48
enum LEDs {
		BALL_1=0, BALL_2, BALL_3, BALL_4, BALL_5,
		LEFT_CAPTURE_LIGHT,RIGHT_CAPTURE_LIGHT,TOP_CAPTURE_LIGHT,
		RED_TARGET_LEFT,RED_TARGET_RIGHT,RED_TARGET_TOP,RED_TARGET_BOTTOM,
		LANE_1,LANE_2,LANE_3,LANE_4,
		LEFT_POP_LIGHT,RIGHT_POP_LIGHT_1,RIGHT_POP_LIGHT_2,RIGHT_POP_LIGHT_3,
		SHOOT_AGAIN,
		LOCK_1,LOCK_2,LOCK_3,LOCK_4,
		HOLD_1,HOLD_2,HOLD_3,HOLD_4,
		LOCK_BALL,START_LOCK,JACKPOT,BONUS_HOLD_INCREMENT,
};
void setLed(enum LEDs index,uint8_t state);
void setPWM(enum LEDs index, uint8_t pwm);
void setPWMFunc(enum LEDs index, uint8_t (*pwmFunc)(void*), void *data);
void offsetLed(enum LEDs index,uint32_t offset);
uint8_t getLed(enum LEDs index);

void updateSlowInputs();
void setHeldRelay(int n,uint8_t state);

#endif /* IO_H_ */
