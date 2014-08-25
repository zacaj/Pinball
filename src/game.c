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

void threeDown(void *data)
{
	DropBank *bank=data;
	if(bank->flashing==bank->nTarget)
	{
		enum LEDs led;
		if(bank->reset==&LEFT_DROP_RESET)
			led=LEFT_CAPTURE_LIGHT;
		else if(bank->reset==&RIGHT_DROP_RESET)
			led=RIGHT_CAPTURE_LIGHT;
		else
			led=TOP_CAPTURE_LIGHT;

		switch(getLed(led))
		{
		case OFF:
			setLed(led,FLASHING);
			break;
		case FLASHING:
			setLed(led,ON);
			break;
		case ON:
			curScore+=drop_complete_score*drop_sequence_mult;
			break;
		}
	}
}

void fiveDown(void *data)
{
	DropBank *bank=data;

}

uint16_t playerScore[];
uint8_t curPlayer=0;
uint16_t bonus=0;
uint16_t physicalBonus=0;
uint16_t physicalScore[4];
uint8_t ballNumber=1,nPlayer=0;
uint8_t bonusMult=0;
uint8_t leftPopScore1=1,leftPopScore2=1;
uint8_t rightPopScore1=1,rightPopScore2=1;
uint32_t lastLeftPopHitTime=0;
uint32_t lastRightPopHitTime=0;
uint8_t extraBallCount=0;
uint8_t nBallInPlay,nBallCaptured,nMinTargetBallInPlay;
uint32_t lastBallTroughReleaseTime=0;
enum GameMode {PLAYER_SELECT=0,SHOOT,PLAY,DRAIN};
enum GameMode mode=PLAYER_SELECT;

void addScore(uint16_t score,uint16_t _bonus)
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
Target redTargets[4]={
		{RED_TARGET_LEFT,0},
		{RED_TARGET_RIGHT,0},
		{RED_TARGET_TOP,0},
		{RED_TARGET_BOTTOM,0},
};
Target lanes[4]={
		{LANE_1,0},
		{LANE_2,0},
		{LANE_3,0},
		{LANE_4,0},
};

void initGame()
{
}

void updateDropBank(DropBank *bank)
{
	int down=0;
	for(int i=0;i<bank->nTarget;i++)
		if(bank->in[i].state)
			down++;
	if(down==bank->nTarget)
	{
		fireSolenoid(bank->reset);
	}
	for(int i=0;i<bank->nTarget;i++)
	{
		if(bank->in[i].pressed)
		{
			addScore(drop_score*(bank->flashing==i?drop_sequence_mult:1),bank->flashing==i?drop_sequence_bonus:0);
			if(bank->flashing==i)//was flashing
			{
				setLed(bank->led[bank->flashing],ON);
				bank->flashing++;
				setLed(bank->led[bank->flashing],FLASHING);
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
					setLed(bank->led[i],OFF);
				setLed(bank->led[bank->flashing],FLASHING);
			}
		}
	}
}

void startGame()
{
	if(nPlayer==0)
		return;

	for(int i=0;i<MAX_PLAYER;i++)
	{
		playerScore[i]=0;
	}


}

void startShoot()
{
	setHeldRelay(BALL_RELEASE,1);
	uint32_t start=msElapsed;
	while(!getIn(BALL_LOADED.pin) && msElapsed-start<3000);
	setHeldRelay(BALL_RELEASE,0);
	lastBallTroughReleaseTime=msElapsed;
	mode=SHOOT;
	setHeldRelay(BALL_SHOOT_ENABLE,1);
}
void resetRedTargets();
void resetLanes();
void startBall()
{
	nBallInPlay=nBallCaptured=nMinTargetBallInPlay=0;
	for(int i=0;i<3;i++)
	{
		captureState[i]=0;
	}
	setLed(LEFT_CAPTURE_LIGHT,OFF);
	setLed(RIGHT_CAPTURE_LIGHT,OFF);
	setLed(TOP_CAPTURE_LIGHT,OFF);

	resetRedTargets();
	resetLanes();
	bonusMult=0;
	leftPopScore1=1;
	leftPopScore2=1;
	rightPopScore1=1;
	rightPopScore2=1;
	startShoot();
}

void extraBall()
{
	extraBallCount++;
	if(extraBallCount>max_extra_balls_per_ball)
		extraBallCount=max_extra_balls_per_ball;
	setLed(SHOOT_AGAIN,ON);
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

void resetLanes()
{
	for(int i=0;i<4;i++)
	{
		lanes[i].state=0;
		setLed(lanes[i].led,OFF);
	}
}

void switchPlayerRelay(int n)
{
	for(int i=0;i<4;i++)
		heldRelayState[PLAYER_ENABLE[i]]=0;
	setHeldRelay(n==-1?PLAYER_ENABLE[0]:n,n==-1?0:1);
}

void startDrain()
{
	mode=DRAIN;
	for(int i=0;i<3;i++)
		captureState[i]=0;
	addScore(bonus*bonusMult,0);
	bonus=0;
}

void nextPlayer()
{
	curPlayer++;
	switchPlayerRelay(curPlayer);
	if(curPlayer>=nPlayer)
	{
		curPlayer=0;
		ballNumber++;
		if(ballNumber>=3)
		{
			switchPlayerRelay(-1);
			mode=PLAYER_SELECT;
		}
	}
}

void updateGame()
{
	if(mode==SHOOT)
	{
		if(BALL_LOADED.pressed)
			setHeldRelay(BALL_SHOOT_ENABLE,1);
		else if(BALL_LOADED.released)
			setHeldRelay(BALL_SHOOT_ENABLE,0);
		if(CAB_LEFT.pressed || CAB_RIGHT.pressed)
		{
			fireSolenoid(&BALL_SHOOT);
		}
		if(LANES[3].pressed)
		{
			mode=PLAY;
			nBallInPlay++;
		}
	}
	if(mode==DRAIN)
	{
		if(BALL_LOADED.state && physicalScore==curScore && physicalBonus==0)
		{
			nextPlayer();
			startBall();
		}
	}
	{//ack
		if(BALL_OUT.state)
			fireSolenoid(&BALL_ACK);
		if(mode==PLAY)
		{
			if(BALL_OUT.pressed)
			{
				if(extraBallCount>0)
				{
					extraBallCount--;
					startShoot();
					nBallInPlay--;
				}
				else if(nMinTargetBallInPlay==0)
				{
					startDrain();
					nBallInPlay--;
				}
			}
		}
	}
	if(mode==PLAY)
	{
		if(nMinTargetBallInPlay>nBallInPlay)
		{
			if(nBallCaptured>0)
			{
				if(LEFT_CAPTURE.state)
					fireSolenoid(&LEFT_CAPTURE_EJECT);
				else if(RIGHT_CAPTURE.state)
					fireSolenoid(&RIGHT_CAPTURE_EJECT);
				else if(TOP_CAPTURE.state)
					fireSolenoid(&TOP_CAPTURE_EJECT);
				else
					nBallCaptured=0;
			}
			else
			{
				if(BALL_LOADED.state)
				{
					setHeldRelay(BALL_RELEASE,0);
					lastBallTroughReleaseTime=msElapsed;
					fireSolenoid(&BALL_SHOOT);
				}
				else if(msElapsed-lastBallTroughReleaseTime>500)
				{
					if(!heldRelayState[BALL_RELEASE])
						setHeldRelay(BALL_RELEASE,1);
				}
			}
		}
	}
	{//captures
		if(TOP_CAPTURE.pressed)
		{
			addScore(capture_score,0);
			nBallInPlay--;
			nBallCaptured++;
		}
		if(TOP_CAPTURE.released)
		{
			nBallCaptured--;
		}
		if(TOP_CAPTURE.state && !captureState[2])
		{
			fireSolenoid(&TOP_CAPTURE_EJECT);
		}

		if(RIGHT_CAPTURE.pressed)
		{
			addScore(capture_score,0);
			nBallCaptured++;
		}
		if(RIGHT_CAPTURE.released)
		{
			nBallCaptured--;
		}
		if(RIGHT_CAPTURE.state && !captureState[1])
		{
			fireSolenoid(&RIGHT_CAPTURE_EJECT);
		}

		if(LEFT_CAPTURE.pressed)
		{
			addScore(capture_score,0);
			nBallCaptured++;
		}
		if(LEFT_CAPTURE.released)
		{
			nBallCaptured--;
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
					addScore(red_target_miss_score,0);
					break;
				case 1:
					{
						redTargets[i].state=2;
						int on=0;
						for(int j=0;j<4;j++)
							if(redTargets[i].state==2)
								on++;
						addScore(red_target_hit_score,on==4?red_target_complete_bonus:0+red_target_hit_bonus);

						setLed(redTargets[i].led,ON);
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
	if(mode==PLAY)//lanes
	{
		for(int i=0;i<4;i++)
		{
			if(LANES[i].pressed)
			{
				if(lanes[i].state==0)
				{
					lanes[i].state=1;
					int on=0;
					for(int j=0;j<4;j++)
						if(lanes[i].state==1)
							on++;
					if(on==4)
					{
						resetLanes();
						bonusMult++;
						if(bonusMult%bonus_mult_extra_ball_divisor==0)
							extraBall();
					}
					addScore(lane_hit_score,on==4?lane_complete_bonus:0);
					setLed(lanes[i].led,ON);
				}
				else
					addScore(lane_miss_score,0);
			}
		}
		if(LEFT_FLIPPER.pressed)
		{
			uint8_t temp=lanes[0].state;
			lanes[0].state=lanes[3].state;
			lanes[3].state=lanes[2].state;
			lanes[2].state=lanes[1].state;
			lanes[1].state=temp;
			for(int i=0;i<4;i++)
				setLed(lanes[i].led,lanes[i].state?ON:OFF);
		}
		if(RIGHT_FLIPPER.pressed)
		{
			uint8_t temp=lanes[0].state;
			lanes[0].state=lanes[1].state;
			lanes[1].state=lanes[2].state;
			lanes[2].state=lanes[3].state;
			lanes[3].state=temp;
			for(int i=0;i<4;i++)
				setLed(lanes[i].led,lanes[i].state?ON:OFF);
		}
	}
	if(mode==PLAY)//pops
	{
		if(LEFT_POP.pressed)
		{
			addScore(leftPopScore1*10,0);
			uint16_t temp=leftPopScore2;
			leftPopScore2=leftPopScore2+leftPopScore1;
			leftPopScore1=temp;
			lastLeftPopHitTime=msElapsed;
			setLed(LEFT_POP_LIGHT,FLASHING);
		}
		if(msElapsed-lastLeftPopHitTime>left_pop_hit_time)
		{
			leftPopScore1=leftPopScore2=1;
			setLed(LEFT_POP_LIGHT,ON);
		}
		if(RIGHT_POP.pressed)
		{
			addScore(rightPopScore1,0);
			uint16_t temp=rightPopScore2;
			rightPopScore2=rightPopScore2+rightPopScore1;
			rightPopScore1=temp;
			lastRightPopHitTime=msElapsed;
			setLed(RIGHT_POP_LIGHT_1,FLASHING);
			setLed(RIGHT_POP_LIGHT_2,FLASHING);
			offsetLed(RIGHT_POP_LIGHT_2,4294967295/3);
			setLed(RIGHT_POP_LIGHT_3,FLASHING);
			offsetLed(RIGHT_POP_LIGHT_3,4294967295/3*2);
		}
		if(msElapsed-lastRightPopHitTime>right_pop_hit_time)
		{
			rightPopScore1=rightPopScore2=1;
			setLed(RIGHT_POP_LIGHT_1,ON);
			setLed(RIGHT_POP_LIGHT_2,ON);
			setLed(RIGHT_POP_LIGHT_3,ON);
		}
	}

	if(mode==PLAYER_SELECT)
	{
		if(START.pressed)
		{
			startGame();
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
