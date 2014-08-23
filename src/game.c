/*
 * game.c

 *
 *  Created on: Aug 23, 2014
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
#include "game.h"

void threeDown(DropBank *bank)
{
	if(bank->flashing==bank->nTarget)
	{
		enum LEDs led;
		switch(bank->reset)
		{
		case &LEFT_DROP_RESET:
			led=LEFT_CAPTURE_LIGHT;
			break;
		case &RIGHT_DROP_RESET:
			led=RIGHT_CAPTURE_LIGHT;
			break;
		case &TOP_DROP_RESET:
			led=TOP_CAPTURE_LIGHT;
			break;
		}
		switch(getLED(led))
		{
		case OFF:
			setLED(led,FLASHING);
			break;
		case FLASHING:
			setLED(led,ON);
			break;
		case ON:
			curScore+=drop_complete_score*drop_sequence_mult;
			break;
		}
	}
}

void fiveDown(DropBank *bank)
{

}

uint16_t playerScore[];
uint8_t curPlayer=0;
uint16_t bonus=0;
DropBank dropBanks[4]={
		{3,DROP_TARGET[0],{0,0,0,0,0},0,&LEFT_DROP_RESET,threeDown},
		{3,DROP_TARGET[1],{0,0,0,0,0},0,&RIGHT_DROP_RESET,threeDown},
		{3,DROP_TARGET[2],{0,0,0,0,0},0,&TOP_DROP_RESET,threeDown},
		{5,FIVE_TARGET,{0,0,0,0,0},0,&FIVE_DROP_RESET,fiveDown},
};


void initGame()
{
	for(int i=0;i<NUM_PLAYER;i++)
	{
		playerScore[i]=0;
	}
}

void updateDropBank(DropBank *bank)
{
	int down=0;
	for(int i=0;i<bank->nTarget;i++)
		if(bank->in[i]->state)
			down++;
	if(down==bank->nTarget)
	{
		fireSolenoid(bank->reset);
	}
	for(int i=0;i<bank->nTarget;i++)
	{
		if(bank->in[i]->pressed)
		{
			curScore+=drop_score*(bank->flashing==i?drop_sequence_mult:1);
			if(bank->flashing==i)
			{
				bonus+=drop_sequence_bonus;//was flashing
				bank->flashing++;
			}
			if(down==bank->nTarget)//last one
			{
				bank->down(bank);
				bonus+=drop_complete_bonus;
				curScore+=drop_complete_score*(bank->flashing==i?drop_sequence_mult:1);
			}
			if(bank->flashing>=bank->nTarget)
			{
				bank->flashing=0;
			}
		}
	}
}

void updateGame()
{
	{//captures
		if(TOP_CAPTURE.pressed)
		{
			curScore+=capture_score;
		}
		if(TOP_CAPTURE.state)
		{
			fireSolenoid(&TOP_CAPTURE_EJECT);
		}

		if(RIGHT_CAPTURE.pressed)
		{
			curScore+=capture_score;
		}
		if(RIGHT_CAPTURE.state)
		{
			fireSolenoid(&RIGHT_CAPTURE_EJECT);
		}

		if(LEFT_CAPTURE.pressed)
		{
			curScore+=capture_score;
		}
		if(LEFT_CAPTURE.state)
		{
			fireSolenoid(&LEFT_CAPTURE_EJECT);
		}
	}
	{//drop
		for(int i=0;i<4;i++)
			updateDropBank(&dropBanks[i]);
	}
}
