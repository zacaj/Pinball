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
#define bmA 1
#define bmB 2
#define bmC 3
#define bmD 4
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
static const IOPin MULTI_IN_LATCH[nMultiInput]= { {bA,P0},{bA,P0},{bA,P0},{bA,P0} };
static const IOPin MULTI_IN_CLOCK[nMultiInput]= { {bA,P0},{bA,P0},{bA,P0},{bA,P0} };
static const IOPin MULTI_IN_DATA[nMultiInput]=  { {bA,P0},{bA,P0},{bA,P0},{bA,P0} };
static const IOPin LED_LATCH={bA,P0};
static const IOPin LED_DATA={bA,P0};
static const IOPin LED_CLOCK={bA,P0};

static const IOPin HOLD={bA,P0};
static const IOPin PLAYFIELD_DISABLE={bA,P0};
static const int PLAYER_ENABLE[]={0,1,2,3};
static const IOPin SCORE[]={{bA,P0},{bA,P0},{bA,P0},{bA,P0}};
static const IOPin SCORE_ZERO[]={{bA,P0},{bA,P0},{bA,P0},{bA,P0}};

#define nHeldRelay 4
static const IOPin heldRelays[nHeldRelay]={{/*Player enable 1*/bA,P0},{bA,P0},{bA,P0},{bA,P0/*Player enable 4*/}};
static const uint8_t heldRelayState[nHeldRelay];
void initInput(IOPin pin, GPIOPuPd_TypeDef def);
void initOutput(IOPin pin);
void setOut(IOPin pin, uint32_t value);
void setOutDirect(IOPin pin, uint32_t value);
uint8_t getIn(IOPin pin);
void initIOs();
void updateIOs();

void fireSolenoidFor(IOPin pin, uint32_t ms);
void fireSolenoid(IOPin pin);


#define OFF 0
#define ON 1
#define FLASHING 2
void setLED(uint8_t index,uint8_t state);
uint8_t getLED(uint8_t index);

void updateSlowInputs();

#endif /* IO_H_ */
