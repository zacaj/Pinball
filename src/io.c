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

uint8_t mInputState[4];
uint8_t mLEDState[6];
uint8_t LED_Dirty=0;

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
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOE, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOF, ENABLE);

}

void updateIOs()
{/*
	for(int i=0;i<4;i++)
	{
		uint8_t in=0;
		setOut(MULTI_IN_LATCH[i],0);
		int i;
		for(i=0;i<8;i++)
		{
			setOut(MULTI_IN_CLOCK[i],0);
			in<<=1;
			in|=getIn(MULTI_IN_DATA[i]);
			setOut(MULTI_IN_CLOCK[i],1);
		}
		setOut(MULTI_IN_LATCH[i],1);
		mInputState[i]=in;
	}
	if(LED_Dirty)
	{
		for(int j=0;j<6;j++)
			for(int i=0;i<8;i++)
			{
				setOut(GPIOD,LED_DATA,mLEDState[j] & 1<<i);
				setOut(GPIOD,LED_CLOCK,1);
				setOut(GPIOD,LED_CLOCK,0);
			}
		setOut(GPIOD,LED_DATA,0);
		setOut(GPIOD,LED_LATCH,1);
		setOut(GPIOD,LED_LATCH,0);
		LED_Dirty=0;
	}*/
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