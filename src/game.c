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
#include "score.h"
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
uint8_t lockMBMax=0;
uint8_t nLock=0;
uint8_t hold=0;
uint16_t startScore=0;
uint32_t startTime=0;
uint8_t activeActivate=0;
uint16_t currentJackpotScore=0;
uint16_t scoreMult=1;
uint8_t nBallInPlay,nBallCaptured,nMinTargetBallInPlay;
uint32_t lastBallTroughReleaseTime=0;
uint32_t lastLeftBlockerOnTime=0,lastRightBlockerOnTime=0;
uint32_t lastLeftBlockerOffTime=0,lastRightBlockerOffTime=0;
uint8_t currentCapture=0;
enum GameMode {PLAYER_SELECT=0,SHOOT,PLAY,DRAIN};
enum GameMode mode=PLAYER_SELECT;
uint8_t p_captureState[4][3];
uint16_t p_bonus[4];
uint8_t p_bonusMult[4];
uint8_t p_nLock[4];
uint8_t p_activateStates[4][4];

void addScore(uint16_t score,uint16_t _bonus)
{
	if(mode==PLAY)
	{
		curScore+=score*scoreMult;
		bonus+=_bonus;
		if(curScore>startScore+consolation_score && msElapsed-startTime>consolation_time)
			if(getLed(SHOOT_AGAIN)==FLASHING)
				setLed(SHOOT_AGAIN,OFF);
	}
}

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
Target activates[4]={
		{LOCK_BALL,0},
		{START_LOCK,0},
		{JACKPOT,0},
		{BONUS_HOLD_INCREMENT,0}
};

void syncCaptureLights();
void rotateActivates();
uint8_t captureState[3];
void threeDown(void *data)
{
	DropBank *bank=data;
	if(bank->flashing==bank->nTarget)
	{
		int n;
		if(bank->reset==&LEFT_DROP_RESET)
		{
			n=0;
		}
		else if(bank->reset==&RIGHT_DROP_RESET)
		{
			n=1;
		}
		else
		{
			n=2;
		}

		switch(captureState[n])
		{
		case 0:
			captureState[n]=1;
			break;
		case 1:
			captureState[n]=2;
			break;
		case 2:
		case 3:
			curScore+=drop_complete_score*drop_sequence_mult;
			break;
		}
		syncCaptureLights();
	}
}

void fiveDown(void *data)
{
	DropBank *bank=data;
	if(bank->flashing==bank->nTarget)
	{
		activates[3].state=1;
		rotateActivates();
	}
	activates[0].state=1;
	rotateActivates();
}


DropBank dropBanks[4]={
		{3,DROP_TARGET[0],{0,0,0,0,0},0,&LEFT_DROP_RESET,threeDown},
		{3,DROP_TARGET[1],{0,0,0,0,0},0,&RIGHT_DROP_RESET,threeDown},
		{3,DROP_TARGET[2],{0,0,0,0,0},0,&TOP_DROP_RESET,threeDown},
		{5,FIVE_TARGET,{0,0,0,0,0},0,&FIVE_DROP_RESET,fiveDown},
};

void initGame()
{
	for(int i=0;i<4;i++)
	{
		for(int j=0;j<3;j++)
			p_captureState[i][j]=0;
		for(int j=0;j<4;j++)
			p_activateStates[i][j]=0;
		p_bonus[i]=0;
		p_bonusMult[i]=0;
		p_nLock[i]=0;
	}
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
	bonus=0;
	bonusMult=0;
	resetScores();

}

void startShoot()
{
	startScore=curScore;
	setLed(SHOOT_AGAIN,FLASHING);
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
void setLocks(int lock)
{
	nLock=lock;
	if(nLock>4)
		nLock=4;
	for(int i=1;i<=4;i++)
		setLed(LOCK_1+i-1,i<=nLock?ON:OFF);
}
void setHold(int _hold)
{
	hold=_hold;
	if(hold>4)
	{
		hold=4;
	}
	for(int i=1;i<=4;i++)
		setLed(HOLD_1+i-1,i<=hold?ON:OFF);
}
void startBall()
{
	nBallInPlay=nBallCaptured=nMinTargetBallInPlay=0;
	for(int i=0;i<3;i++)
	{
		captureState[i]=p_captureState[curPlayer][i];
	}
	syncCaptureLights();

	resetRedTargets();
	resetLanes();
	scoreMult=1;
	bonusMult=p_bonusMult[curPlayer];
	leftPopScore1=1;
	leftPopScore2=1;
	rightPopScore1=1;
	rightPopScore2=1;
	lastLeftBlockerOnTime=0;
	lastRightBlockerOnTime=0;
	lastLeftBlockerOffTime=0;
	lastRightBlockerOffTime=0;
	setHeldRelay(LEFT_BLOCK_DISABLE,0);
	setHeldRelay(RIGHT_BLOCK_DISABLE,0);
	setLocks(p_nLock[curPlayer]);
	setHold(0);
	bonus=p_bonus[curPlayer];
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

void syncCaptureLights()
{
#define sync(led,n) \
	switch(captureState[n]) \
	{ \
	case 0: \
		setLed(led,OFF); \
		break; \
	case 1: \
		setLed(led,FLASHING); \
		break; \
	case 2: \
	case 3: \
		setLed(led,ON); \
		break; \
	}

	sync(LEFT_CAPTURE_LIGHT,0);
	sync(RIGHT_CAPTURE_LIGHT,1);
	sync(TOP_CAPTURE_LIGHT,2);
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
	p_bonus[curPlayer]=bonus*hold/4;
	bonus=0;
}

void rotateActivates()
{
	setLed(activates[activeActivate].led,OFF);
	activeActivate++;
	if(activeActivate>=4)
		activeActivate=0;
	setLed(activates[activeActivate].led,activates[activeActivate].state);
}

void nextPlayer()
{
	p_bonusMult[curPlayer]=bonusMult*hold/4;
	p_nLock[curPlayer]=nLock>0?nLock-1:0;
	for(int i=0;i<4;i++)
		p_activateStates[curPlayer][i]=activates[i].state;
	captureState[currentCapture]--;
	for(int i=0;i<3;i++)
		p_captureState[curPlayer][i]=captureState[i]>0?captureState[i]-1:0;
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
			startTime=msElapsed;
			nBallInPlay++;
		}
	}
	if(mode==DRAIN)
	{
		if(BALL_LOADED.state && physicalScore[curPlayer]==curScore && physicalBonus==0)
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
				if(nBallInPlay==1)//single ball
				{
					lockMBMax=0;
					if(extraBallCount>0)
					{
						extraBallCount--;
						nBallInPlay--;
						startShoot();
					}
					else if(nMinTargetBallInPlay==0)
					{
						startDrain();
						nBallInPlay--;
					}
				}
				else if(nBallInPlay>1) //multi-ball
				{
					if(nBallInPlay<lockMBMax && nLock>0)
					{
						nMinTargetBallInPlay=nBallCaptured+nBallInPlay+1;
					}
					else if(nBallInPlay==2)
					{
						lockMBMax=0;
					}
					nBallInPlay--;
					if(scoreMult!=1)
						scoreMult=nBallInPlay;
				}
			}
		}
	}
	if(mode==PLAY)
	{
		if(nMinTargetBallInPlay>nBallInPlay)
		{
			if(nMinTargetBallInPlay-nBallInPlay-nBallCaptured>0)
			{
				if(BALL_LOADED.state)
				{
					setHeldRelay(BALL_RELEASE,0);
					lastBallTroughReleaseTime=msElapsed;
					fireSolenoid(&BALL_SHOOT);
					setLocks(nLock-1);
				}
				else if(msElapsed-lastBallTroughReleaseTime>500)
				{
					if(!heldRelayState[BALL_RELEASE])
						setHeldRelay(BALL_RELEASE,1);
				}
			}
			else
			{
				if(LEFT_CAPTURE.state)
				{
					if(fireSolenoid(&LEFT_CAPTURE_EJECT))
						setLocks(nLock-1);
				}
				else if(RIGHT_CAPTURE.state)
				{
					if(fireSolenoid(&RIGHT_CAPTURE_EJECT))
						setLocks(nLock-1);
				}
				else if(TOP_CAPTURE.state)
				{
					if(fireSolenoid(&TOP_CAPTURE_EJECT))
						setLocks(nLock-1);
				}
			}
		}
		else //done firing
		{
			nMinTargetBallInPlay=0;
			setHeldRelay(MAGNET,0);
		}
	}
	{//captures
#define doCapture(in,eject,n) \
	if(in.pressed) \
	{ \
		addScore(capture_score,0); \
		nBallInPlay--; \
		nBallCaptured++; \
		if(lockMBMax==0) \
		{ \
			if(captureState[n]==1 || (captureState[n]>=2 && nBallCaptured==3)) \
			{ \
				nMinTargetBallInPlay=nBallCaptured+nBallInPlay+1; \
				scoreMult=nMinTargetBallInPlay; \
				for(int i=0;i<3;i++) \
				{ \
					captureState[i]=0; \
				} \
				syncCaptureLights(); \
			} \
		} \
		else \
		{ \
			if(captureState[n]<2) \
			{ \
				captureState[n]++; \
				syncCaptureLights(); \
			} \
			if(captureState[0]>=2 && captureState[1]>=2 && captureState[2]>=2) \
			{ \
				activates[2].state=2; \
				rotateActivates(); \
			} \
		} \
	} \
	if(in.released) \
	{ \
		nBallCaptured--; \
	} \
	if(in .state) \
	{ \
		switch(captureState[n]) \
		{ \
		case 0: \
			fireSolenoid(&eject); \
			break; \
		case 1: \
			break; \
		case 2: \
			break; \
		} \
	}

		doCapture(TOP_CAPTURE,TOP_CAPTURE_EJECT,2);
		doCapture(LEFT_CAPTURE,LEFT_CAPTURE_EJECT,0);
		doCapture(RIGHT_CAPTURE,RIGHT_CAPTURE_EJECT,1);
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
	if(mode==PLAY)
	{
		if(ROTATE_ROLLOVER.pressed)
		{
			rotateActivates();
		}
		if(ACTIVATE_TARGET.pressed)
		{
			if(activates[activeActivate].state)
			{
				addScore(0,1);
				switch(activeActivate)
				{
				case 0:
					setLocks(nLock+1);
					activates[0].state=0;
					break;
				case 1:
					nMinTargetBallInPlay=nBallCaptured+nBallInPlay+nLock;
					lockMBMax=nLock;
					setHeldRelay(MAGNET,1);
					currentJackpotScore=jackpot_score;
					activates[1].state=0;
					break;
				case 2:
					addScore(currentJackpotScore,jackpot_bonus);
					currentJackpotScore*=jackpot_score_mult;
					activates[2].state=0;
					break;
				case 3:
					setHold(hold+1);
					activates[3].state=0;
					break;
				}
			}
			else
				addScore(5,0);
			rotateActivates();
		}
		if(nLock>=nBallCaptured)
		{
			activates[1].state=2;
		}
		else
		{
			activates[1].state=0;
		}
	}
	if(mode==PLAY)//blockers
	{
		if(LEFT_BLOCK.pressed)
			lastLeftBlockerOnTime=msElapsed;
		if(RIGHT_BLOCK.pressed)
			lastRightBlockerOnTime=msElapsed;
		if(LEFT_BLOCK.released)
		{
			lastLeftBlockerOnTime=0;
			lastLeftBlockerOffTime=msElapsed;
		}
		if(RIGHT_BLOCK.released)
		{
			lastRightBlockerOnTime=0;
			lastRightBlockerOffTime=msElapsed;
		}
		if(msElapsed-lastLeftBlockerOnTime>max_blocker_on_time)
		{
			setHeldRelay(LEFT_BLOCK_DISABLE,1);
			lastLeftBlockerOffTime=msElapsed;
		}
		if(msElapsed-lastRightBlockerOnTime>max_blocker_on_time)
		{
			setHeldRelay(RIGHT_BLOCK_DISABLE,1);
			lastRightBlockerOffTime=msElapsed;
		}
		if(msElapsed-lastLeftBlockerOffTime>blocker_cooldown_time)
		{
			setHeldRelay(LEFT_BLOCK_DISABLE,0);
		}
		if(msElapsed-lastRightBlockerOffTime>blocker_cooldown_time)
		{
			setHeldRelay(RIGHT_BLOCK_DISABLE,0);
		}
	}
	if(mode==PLAY)
	{
		if(BUMPER.pressed)
		{
			captureState[currentCapture]--;
			currentCapture++;
			if(currentCapture>2)
				currentCapture=0;
			captureState[currentCapture]++;
			syncCaptureLights();
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
