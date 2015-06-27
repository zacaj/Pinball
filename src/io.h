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
	GPIO_TypeDef* const bank;
	const uint32_t pin;
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
	const IOPin pin;
	uint32_t lastFired, lastLastFired;
	uint32_t onTime,temp,offTime;
	uint8_t waitingToFire;
	uint8_t state;
} Solenoid;
#define S(b,p) {{b,p},-250, -250,90,250,0,0}
#define Sd(b,p,on) {{b,p},-250, -250,on,250,0,0}
#define Sds(b,p,on,off) {{b,p},-off, -off,on,0,off,0,0}

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
		500,
		8000,
		-1,-1,-1

};

typedef struct
{
	const IOPin pin;
	uint8_t state;
	uint32_t lastChange,lastRawChange;
	uint8_t pressed;
	uint8_t released;
	uint8_t inverse;
	uint8_t rawState;
	uint32_t settleTime;
	uint32_t lastOn,lastOff;
	uint32_t lastRawOn,lastRawOff;
} Input;
#define In(b,p) {{b,p},0,1,1,0,0,0,0,1,0,0,0,0}

extern Input DROP_TARGET[3][3];
extern Input FIVE_TARGET[5];
extern Input LEFT_CAPTURE;
extern Input RIGHT_CAPTURE;
extern Input TOP_CAPTURE;
extern Input SCORE_ZERO[4];
extern Input BONUS_ZERO[4];
extern Input BALL_OUT;
extern Input SHOOT_BUTTON;
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
void fireSolenoidAlways(Solenoid *s);

uint8_t fireSolenoidFor(Solenoid *s, uint32_t ms);
uint8_t fireSolenoid(Solenoid *s);
void fireSolenoidIn(Solenoid *s, uint32_t ms);

#define OFF 0
#define ON 1
#define FLASHING 2
#define PWM 3
#define nLED 48
enum LEDs {
	TOP_1=0, TOP_2, TOP_3, TOP_CAPTURE_LIGHT, RED_TARGET_TOP,
	FIVE_2, RED_TARGET_BOTTOM, BALL_1, FIVE_1, FIVE_5, FIVE_3,
	FIVE_4, RIGHT_POP_LIGHT_1, LEFT_CAPTURE_LIGHT, RIGHT_POP_LIGHT_2, RIGHT_POP_LIGHT_3,
	LEFT_3, RED_TARGET_RIGHT, RED_TARGET_LEFT, RIGHT_2,
	LOCK_1, SHOOT_AGAIN, LOCK_3, LOCK_2, LANE_3, RIGHT_CAPTURE_LIGHT,
	LANE_4, LOCK_4, RIGHT_3, LEFT_1, RIGHT_1, LEFT_2,
	BALL_2, BALL_3, BALL_4, BALL_5, RED_LED, WHITE_LED, PURPLE_LED,
	LEFT_POP_LIGHT, HOLD_1, LANE_2, LANE_1, YELLOW_LED, HOLD_2,
	HOLD_3, HOLD_4, TEMP_BALL_LAUNCH_READY
};
uint8_t setLed(enum LEDs index,uint8_t state);
uint8_t setFlash(enum LEDs index, uint32_t max);
void setPWM(enum LEDs index, uint8_t pwm);
void setPWMFunc(enum LEDs index, uint8_t (*pwmFunc)(void*), void *data);
void offsetLed(enum LEDs index,uint32_t offset);
uint8_t getLed(enum LEDs index);
void syncLeds(enum LEDs a, enum LEDs b, enum LEDs c, enum LEDs d);

enum Command {
	cSTART=0,
	cJACKPOT,
	cUSE_EXTRA_BALL,
	cBALL_OUT,
	cEXTRA_BALL,
	cCOLLECT_BONUS,
	cLOCK_START,
	cBALL_CAPTURED_MB,
	cBALL_CAPTURED_LOCK,
	cJACKPOT_READY,
	cEXTRA_BALL_READY,
	cCOLLECT_BONUS_READY,
	cBALLS_LOCKED,
	cMB_READY,
	cLANE_COMPLETE,
	cSHOOT,
	cTHREE_COMPLETE,
	cBALL_CAPTURED_REJECTED,
	cFIVE_HIT,
	cSHOOT_CAREFULLY,
	cRED_HIT,
	cLANE_HIT,
	cBALL_CAPTURE_EJECT,
	cTHREE_HIT,
	cACTIVATE_MISS,
	cROLLOVER,
	cRED_MISS,
	cFIVE_MISS,
	cPOP_HIT,
	cTHREE_MISS,
	cLANE_MISS
};

void updateSlowInputs();
void setHeldRelay(int n,uint8_t state);

uint8_t sendCommand(uint8_t cmd);
uint8_t sendCommandElse(uint8_t cmd);
#endif /* IO_H_ */
