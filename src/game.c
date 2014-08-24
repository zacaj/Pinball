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
uint8_t ballNumber=1,nPlayer=0;
enum GameMode {PLAYER_SELECT=0,SHOOT,PLAY,DRAIN};
enum GameMode mode=PLAYER_SELECT;

void addScore(uint16_t score,uint16_t _bonus=0)
{
	if(mode==PLAY)
	{
		curScore+=score;
		bonus+=_bonus;
	}
}

DropBank dropBanks[4]={
		{3,DROP_TARGET[0],{0,0,0,0,0},0,&LEFT_DROP_RESET,threeDown},
		{3,DROP_TARGET[1],{0,0,0,0,0},0,&RIGHT_DROP_RESET,threeDown},
		{3,DROP_TARGET[2],{0,0,0,0,0},0,&TOP_DROP_RESET,threeDown},
		{5,FIVE_TARGET,{0,0,0,0,0},0,&FIVE_DROP_RESET,fiveDown},
};
uint8_t captureState[3];
RedTarget redTargets[4]={
		{RED_TARGET_LEFT,0},
		{RED_TARGET_RIGHT,0},
		{RED_TARGET_TOP,0},
		{RED_TARGET_BOTTOM,0},
};

void initGame()
{
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
			addScore(drop_score*(bank->flashing==i?drop_sequence_mult:1),bank->flashing==i?drop_sequence_bonus:0);
			if(bank->flashing==i)//was flashing
			{
				setLED(bank->led[bank->flashing],ON);
				bank->flashing++;
				setLED(bank->led[bank->flashing],FLASHING);
			}
			if(down==bank->nTarget)//last one
			{
				bank->down(bank);
				addScore(drop_complete_score*(bank->flashing==i?drop_sequence_mult:1),drop_complete_bonus);
			}
			if(bank->flashing>=bank->nTarget)
			{
				bank->flashing=0;
				for(int i=0;i<bank->nTarget;i++)
					setLED(bank->led[i],OFF);
				setLED(bank->led[bank->flashing],FLASHING);
			}
		}
	}
}

void resetGame()
{
	if(nPlayer==0)
		return;

	for(int i=0;i<MAX_PLAYER;i++)
	{
		playerScore[i]=0;
	}

	for(int i=0;i<3;i++)
	{
		captureState[i]=0;
	}
	setLed(LEFT_CAPTURE_LIGHT,OFF);
	setLed(RIGHT_CAPTURE_LIGHT,OFF);
	setLed(TOP_CAPTURE_LIGHT,OFF);

	resetRedTargets();
}

void extraBall()
{

}

void resetRedTargets()
{
	for(int i=0;i<4;i++)
	{
		setLed(redTargets[i].led,OFF);
		redTargets[i].state=0;
	}
	int i=rand()%4;
	setLed(redTargets[i].led,FLASHING);
	redTargets[i].state=1;
}

void switchPlayerRelay(int n)
{
	for(int i=0;i<4;i++)
		heldRelayState[PLAYER_ENABLE[i]]=0;
	setHeldRelay(n,1);
}

void updateGame()
{
	{//captures
		if(TOP_CAPTURE.pressed)
		{
			addScore(capture_score);
		}
		if(TOP_CAPTURE.state && !captureState[2])
		{
			fireSolenoid(&TOP_CAPTURE_EJECT);
		}

		if(RIGHT_CAPTURE.pressed)
		{
			addScore(capture_score);
		}
		if(RIGHT_CAPTURE.state && !captureState[1])
		{
			fireSolenoid(&RIGHT_CAPTURE_EJECT);
		}

		if(LEFT_CAPTURE.pressed)
		{
			addScore(capture_score);
		}
		if(LEFT_CAPTURE.state && !captureState[0])
		{
			fireSolenoid(&LEFT_CAPTURE_EJECT);
		}
	}
	if(mode==PLAY)
	{//drop
		for(int i=0;i<4;i++)
			updateDropBank(&dropBanks[i]);
	}
	if(mode==PLAY)
	{//red
		for(int i=0;i<4;i++)
		{
			if(RED_TARGET[i].pressed)
			{
				switch(redTargets[i].state)
				{
				case 0:
				case 2:
					addScore(red_target_miss_score);
					break;
				case 1:
					{
						addScore(red_target_hit_score);
						redTargets[i].state=2;
						setLed(redTargets[i].led,ON);
						int on=0;
						for(int j=0;j<4;j++)
							if(redTargets[i].state==2)
								on++;
						if(on==4)
						{
							resetRedTargets();
							extraBall();
						}
						else
						{
							int j;
							do
							{
								j=rand()%4;
							} while(redTargets[j].state==2);
							redTargets[j].state=1;
							setLed(redTargets[j].led,FLASHING);
						}
					}
					break;
				}
			}
		}
	}
	if(mode==PLAYER_SELECT)
	{
		if(START.pressed)
		{
			resetGame();
		}
		if(CAB_LEFT.pressed)
		{
			if(nPlayer>1)
				nPlayer--;
			switchPlayerRelay(nPlayer);
		}
		if(CAB_RIGHT.pressed)
		{
			if(nPlayer<MAX_PLAYER)
				nPlayer++;
			switchPlayerRelay(nPlayer);
		}
	}

}
