/*
 * io.c
 *
 *  Created on: Jul 16, 2014
 *      Author: zacaj
 */


#include <stddef.h>
#include "stm32f30x.h"
#include "stm32f30x_gpio.h"
#include "stm32f30x_conf.h"
#include "stm32f3_discovery.h"
#include <stdio.h>
#include "io.h"

void initInput(IOPin pin, GPIOPuPd_TypeDef def)
{
	GPIO_InitTypeDef init;
	init.GPIO_Mode = GPIO_Mode_IN;
	init.GPIO_OType = GPIO_OType_PP;
	init.GPIO_Pin = pin.pin;
	init.GPIO_PuPd = def;

	GPIO_Init(pin.bank, &init);
}

void initOutput(IOPin pin)
{
	GPIO_InitTypeDef init;
	init.GPIO_Mode = GPIO_Mode_OUT;
	init.GPIO_OType = GPIO_OType_PP;
	init.GPIO_Pin = pin.pin;
	init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	init.GPIO_Speed = GPIO_Speed_Level_3;
	GPIO_Init(pin.bank, &init);
}

void setOut(IOPin pin, uint32_t value)
{
	GPIO_WriteBit(pin.bank, pin.pin, value);
}

uint8_t getIn(IOPin pin)
{
	return GPIO_ReadInputDataBit(pin.bank, pin.pin);
}

void initIOs()
{

	  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);
	  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOE, ENABLE);
	  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOF, ENABLE);
}
