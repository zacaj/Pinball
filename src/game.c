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
uint16_t ballSaveMinScore=0;
uint32_t ballSaveEndTime=0;
uint8_t activeActivate=0;
uint16_t currentJackpotScore=0;
uint16_t scoreMult=1;
uint8_t nBallInPlay,nBallCaptured,nMinTargetBallInPlay;
uint32_t lastBallTroughReleaseTime=0;
uint32_t lastLeftBlockerOnTime=-1,lastRightBlockerOnTime=-1;
uint32_t lastLeftBlockerOffTime=-1,lastRightBlockerOffTime=-1;
uint8_t currentCapture=0;
enum GameMode {PLAYER_SELECT=0,SHOOT,PLAY,DRAIN};
enum GameMode mode=PLAYER_SELECT;
uint8_t p_captureState[4][3];
uint16_t p_bonus[4];
uint8_t p_bonusMult[4];
uint8_t p_nLock[4];
uint8_t p_activateStates[4][4];

void updateExtraBall()
{
	if(curScore<ballSaveMinScore || msElapsed<ballSaveEndTime)
		flashLed(SHOOT_AGAIN,800);
	else if(extraBallCount>0)
		setLed(SHOOT_AGAIN,ON);
	else
		setLed(SHOOT_AGAIN,OFF);
}
void startBallSave(uint32_t time)
{
	ballSaveEndTime=msElapsed+time;
	updateExtraBall();
}


void addScore(uint16_t score,uint16_t _bonus)
{
	if(mode==PLAY)
	{
		_BREAK();
		curScore+=score*scoreMult;
		bonus+=_bonus*scoreMult;
		updateExtraBall();
	}
}

Target redTargets[4]={
		{RED_TARGET_RIGHT,0},
		{RED_TARGET_LEFT,0},
		{RED_TARGET_BOTTOM,0},
		{RED_TARGET_TOP,0},
};
Target lanes[4]={
		{LANE_1,0},
		{LANE_2,0},
		{LANE_3,0},
		{LANE_4,0},
};
Target activates[4]={
		{BLACKOUT,0},
		{START_LOCK,0},
		{EXTRA_BALL,0},
		{BONUS_HOLD_INCREMENT,0}
};

void syncCaptureLights();
void rotateActivates();
uint8_t captureState[3];
void threeDown(void *data)
{
	DropBank *bank=data;
}

void fiveDown(void *data)
{
	DropBank *bank=data;
	activates[0].state=1;
	rotateActivates();
}


DropBank dropBanks[4]={
		{3,DROP_TARGET[0],{LEFT_1,LEFT_2,LEFT_3,0,0},0,&LEFT_DROP_RESET,0,threeDown},
		{3,DROP_TARGET[1],{RIGHT_1,RIGHT_2,RIGHT_3,0,0},0,&RIGHT_DROP_RESET,0,threeDown},
		{3,DROP_TARGET[2],{TOP_1,TOP_2,TOP_3,0,0},0,&TOP_DROP_RESET,0,threeDown},
		{5,FIVE_TARGET,{FIVE_1,FIVE_2,FIVE_3,FIVE_4,FIVE_5},0,&FIVE_DROP_RESET,0,fiveDown},
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

	for(int i=0;i<nLED;i++)
	{
		//setPWMFunc(i,pwmFunc,i);
		//setPWM(i,255);
		//setLed(i,ON);
		setFlash(i,3000);
		offsetLed(i,3000*(i%2));
		//setLed(i,FLASHING);
	}
	nPlayer=1;
}

void initDropBank(DropBank *bank) {
	int down=0;
	bank->flashing = 0;
	do
	{
		updateIOs();
		down=0;
		for(int i=0;i<bank->nTarget;i++) {
			if(bank->in[i].state)
				down++;
		}
		if (down)
		{
			BREAK();
			bank->resetting = 1;
			fireSolenoid(bank->reset);
			//wait(200);
			_BREAK();
			for (int i = 0; i<bank->nTarget; i++) {
				setLed(bank->led[i], ON);
			}
		}
		else
			break;
	} while(down>0);
	for(int i=0;i<bank->nTarget;i++) {
		setLed(bank->led[i],OFF);
	}
	flashLed(bank->led[bank->flashing],drop_flash_period);
}

void updateDropBank(DropBank *bank)
{
	int down=0;
	for(int i=0;i<bank->nTarget;i++)
		if(bank->in[i].state)
			down++;
	if(!bank->resetting)
	for(int i=0;i<bank->nTarget;i++)
	{
		if(bank->in[i].pressed)
		{
			//BREAK();
			if (bank->nTarget == 5) {
				if (bank->flashing == bank->nTarget)
				{
					activates[3].state = 1;
					rotateActivates();
				}
				setLocks(nLock + 1);
			}
			else if (bank->nTarget == 3) {
				if (bank->flashing == bank->nTarget)
				{
					int n;
					if (bank->reset == &LEFT_DROP_RESET)
					{
						n = 0;
					}
					else if (bank->reset == &RIGHT_DROP_RESET)
					{
						n = 1;
					}
					else
					{
						n = 2;
					}

					switch (captureState[n])
					{
					case 0:
						captureState[n] = 1;
						break;
					case 1:
						captureState[n] = 2;
						break;
					case 2:
					case 3:
						curScore += drop_complete_score*drop_sequence_mult;
						break;
					}
					syncCaptureLights();
				}
			}
			addScore(drop_score*(bank->flashing==i?drop_sequence_mult:1),bank->flashing==i?drop_sequence_bonus:0);
			if(bank->flashing==i)//was flashing
			{
				setLed(bank->led[bank->flashing],ON);
				bank->flashing++;
				flashLed(bank->led[bank->flashing],drop_flash_period);
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
				flashLed(bank->led[bank->flashing],drop_flash_period);
			}
		}
	}
	if(down==bank->nTarget)
	{
		fireSolenoidIn(bank->reset,600);
		bank->resetting=1;
	}
	if(!down)
	{
		bank->resetting=0;
	}
}
void flashLed(enum LEDs led, uint32_t period)
{
	setFlash(led,period*5);
}
void startGame()
{
	if(nPlayer==0)
		return;

	for(int i=0;i<nLED;i++)
	{
		//setPWMFunc(i,pwmFunc,i);
		//setPWM(i,255);
		setLed(i,OFF);
		//setFlash(i,3000);
		//offsetLed(i,3000*(i%2));
		//setLed(i,FLASHING);
	}
	for(int i=0;i<MAX_PLAYER;i++)
	{
		playerScore[i]=0;
	}
	bonus=0;
	bonusMult=0;
	resetScores();
	wait(500);
	for(int i=3;i>=0;i--)
		initDropBank(&dropBanks[i]);
	curPlayer=1;
	switchPlayerRelay(curPlayer);
	startBall();
}

void startShoot()
{
	ballSaveMinScore=curScore;
	setLed(SHOOT_AGAIN,FLASHING);
	//BREAK();
	setHeldRelay(BALL_RELEASE,1);
	wait(250);
	setHeldRelay(BALL_RELEASE,0);
	lastBallTroughReleaseTime=msElapsed;
	mode=SHOOT;
	//BREAK();
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
		hold=0;
		addScore(bonus*bonusMult*2,0);
	}
	for(int i=1;i<=4;i++)
		setLed(HOLD_1+i-1,i<=hold?ON:OFF);
}
void startBall()
{//
	//BREAK();
	nBallInPlay=nBallCaptured=nMinTargetBallInPlay=0;
	for(int i=0;i<3;i++)
	{
		captureState[i]=p_captureState[curPlayer][i];
	}
	syncCaptureLights();
	for (int i = 0; i < 4; i++)
		initDropBank(&dropBanks[i]);
	resetRedTargets();
	resetLanes();
	scoreMult=1;
	bonusMult=p_bonusMult[curPlayer];
	leftPopScore1=1;
	leftPopScore2=1;
	rightPopScore1=1;
	rightPopScore2=1;
	lastLeftBlockerOnTime=-1;
	lastRightBlockerOnTime=-1;
	lastLeftBlockerOffTime=-1;
	lastRightBlockerOffTime=-1;
	setHeldRelay(LEFT_BLOCK_DISABLE,0);
	setHeldRelay(RIGHT_BLOCK_DISABLE,0);
	setLocks(p_nLock[curPlayer]);
	setHold(0);
	bonus=p_bonus[curPlayer];
	//BREAK();
	startShoot();
}

void extraBall()
{
	extraBallCount++;
	if(extraBallCount>max_extra_balls_per_ball)
	{
		extraBallCount=max_extra_balls_per_ball;
		addScore(100,10);
	}
	startBallSave(4000);
	updateExtraBall();
}

void resetRedTargets()
{
	for(int i=0;i<4;i++)
	{
		setLed(redTargets[i].led,OFF);
		redTargets[i].state=0;
	}
	int i=rand()%4;
	flashLed(redTargets[i].led,500);
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
	BREAK();
	for(int i=0;i<4;i++)
		heldRelayState[PLAYER_ENABLE[i]]=0;
	if(n!=-1)
		heldRelayState[PLAYER_ENABLE[n]]=1;
	updateHeldRelays();
}

void startDrain()
{
	BREAK();
	mode=DRAIN;
	for(int i=0;i<3;i++)
		captureState[i]=0;
	addScore(bonus*bonusMult,0);
	p_bonus[curPlayer]=bonus*hold/4;
	p_bonusMult[curPlayer]=bonusMult*hold/4;
	bonus=0;
	bonusMult=0;
}

void rotateActivates()
{
	int i = 0;
	do {
		activeActivate++;
		i++;
		if (activates[activeActivate].state)
			break;
	} while (i < 4);
	if (!activates[activeActivate].state)
		activeActivate = -1;
	for(int i=0;i<4;i++)
	{
		if(activates[activeActivate].state==0)
			setLed(activates[i].led,OFF);
		else if(i==activeActivate)
			flashLed(activates[i].led,400);
		else
			setLed(activates[i].led,ON);
	}
}

void nextPlayer()
{
	BREAK();
	p_nLock[curPlayer]=nLock>0?nLock-1:0;
	for(int i=0;i<4;i++)
		p_activateStates[curPlayer][i]=activates[i].state;
	captureState[currentCapture]--;
	for(int i=0;i<3;i++)
		p_captureState[curPlayer][i]=captureState[i]>0?captureState[i]-1:0;
	curPlayer++;
	if(curPlayer>=nPlayer)
	{
		curPlayer=1;//should be 0
		ballNumber++;
		if(ballNumber>=5)
		{
			curPlayer=-1;
			mode=PLAYER_SELECT;
		}
	}
	switchPlayerRelay(curPlayer);
}
uint32_t turnOffMagnet() {
	setHeldRelay(MAGNET,0);
}
uint8_t waitingToAutoFireBall=0;
void updateGame()
{
	if(mode==SHOOT)
	{
		/*if(SHOOT_BUTTON.pressed)
		{
			fireSolenoid(&BALL_SHOOT);
			setHeldRelay(BALL_SHOOT_ENABLE,0);
		}*/
		if(LANES[3].pressed)
		{
			mode=PLAY;
			startBallSave(consolation_time);
			ballSaveMinScore=curScore+consolation_score;
			updateExtraBall();
			nBallInPlay++;
			//LANES[3].pressed=0;
		}
		if(BALL_OUT.pressed)
		{
			startShoot();
		}
	}
	if(mode==DRAIN)
	{
		if(BALLS_FULL.state && BALL_OUT.state && physicalScore[curPlayer]==curScore && physicalBonus==0)
		{
			nextPlayer();
			startBall();
		}
	}
	{//ack
		if(BALL_OUT.state && !BALLS_FULL.state)
			fireSolenoid(&BALL_ACK);
		if(mode==PLAY)
		{
			if(BALL_OUT.pressed)
			{//hxh 11:00 23
				//if(nBallInPlay==1)//single ball
				{
					lockMBMax=0;
					if(getLed(SHOOT_AGAIN)==FLASHING)
					{
						nBallInPlay--;
						startShoot();
					}
					else if(extraBallCount>0)
					{
						extraBallCount--;
						nBallInPlay--;
						startShoot();
					}
					else //if(nMinTargetBallInPlay==0)
					{
						startDrain();
						nBallInPlay--;
					}
				}
				/*else if(nBallInPlay>1) //multi-ball
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
				}*/
			}
		}
	}
	/*if(mode==PLAY)
	{
		if(nMinTargetBallInPlay>nBallInPlay)
		{
			if(nMinTargetBallInPlay-nBallInPlay-nBallCaptured>0)
			{
				if(waitingToAutoFireBall && msElapsed-lastBallTroughReleaseTime>ball_release_wait_time)
				{
					setHeldRelay(BALL_RELEASE,0);
					//setHeldRelay(BALL_SHOOT_ENABLE,0);
					lastBallTroughReleaseTime=msElapsed;
					fireSolenoid(&BALL_SHOOT);
					startBallSave(consolation_time);
					setLocks(nLock-1);
					waitingToAutoFireBall=0;
				}
				else if(msElapsed-lastBallTroughReleaseTime>500)
				{
					if(!heldRelayState[BALL_RELEASE])
					{
						setHeldRelay(BALL_RELEASE,1);
						//setHeldRelay(BALL_SHOOT_ENABLE,1);
						lastBallTroughReleaseTime=msElapsed;
						waitingToAutoFireBall=1;
					}
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
	}*/
	{//captures
#define doCapture(in,eject,n) \
	if(in.pressed && 0) \
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
		else if(0)\
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
	if(in.released && 0) \
	{ \
		nBallCaptured--; \
	} \
	if(in .state) \
	{ \
		switch(captureState[n]) \
		{ \
		case 0: \
			fireSolenoidIn(&eject,450); \
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
				BREAK();
				switch(redTargets[i].state)
				{
				case 0: // off
				case 2://.5
				case 4://on
					addScore(red_target_miss_score,0);
					break;
				case 1://fast flashing
				case 3:
					{
						redTargets[i].state++;
						int on=0;
						for(int j=0;j<4;j++)
							if(redTargets[j].state==redTargets[i].state)
								on++;
						addScore(red_target_hit_score,on==4?red_target_complete_bonus:0+red_target_hit_bonus);

						if (redTargets[i].state == 4)
							setLed(redTargets[i].led,ON);
						else
							setLed(redTargets[i].led,ON);
							//setPWM(redTargets[i].led,100);

						if (on == 4 && redTargets[i].state == 4)
						{
							activates[2].state=2;
						}
						else
						{
							int j;
							do
							{
								j=rand()%4;
							} while(redTargets[j].state>=redTargets[i].state);
							redTargets[j].state++;
							flashLed(redTargets[j].led,500);
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
						if(lanes[j].state==1)
							on++;
					if(on==4)
					{
						resetLanes();
						bonusMult++;
						if(bonusMult%bonus_mult_extra_ball_divisor==0)
							extraBall();
					}
					else
						setLed(lanes[i].led,ON);
					addScore(lane_hit_score,on==4?lane_complete_bonus:0);
				}
				else
					addScore(lane_miss_score,0);
			}
		}
		if(RIGHT_FLIPPER.pressed)
		{
			uint8_t temp=lanes[0].state;
			lanes[0].state=lanes[3].state;
			lanes[3].state=lanes[2].state;
			lanes[2].state=lanes[1].state;
			lanes[1].state=temp;
			for(int i=0;i<4;i++)
				setLed(lanes[i].led,lanes[i].state?ON:OFF);
		}
		if(LEFT_FLIPPER.pressed)
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
	if (mode == PLAY)
	{
#define doBlackout() \
	setHeldRelay(MAGNET, 1); \
	callFuncIn(turnOffMagnet, 1000, NULL);
#define doLock() \
		if (lockMBMax) \
		{ \
		addScore(currentJackpotScore, jackpot_bonus); \
		currentJackpotScore *= jackpot_score_mult; \
		setHeldRelay(MAGNET, 1); \
		callFuncIn(turnOffMagnet, 1500, NULL); \
	} \
	else \
		{ \
		nMinTargetBallInPlay = nBallCaptured + nBallInPlay + nLock; \
		lockMBMax = nLock; \
		setHeldRelay(MAGNET, 1); \
		currentJackpotScore = jackpot_score; \
		startBallSave(3000); \
	}
#define doExtraBall() \
	resetRedTargets(); \
	extraBall();
#define doHold() \
	setHold(hold + 1); \
	setHeldRelay(MAGNET, 1); \
	callFuncIn(turnOffMagnet, 1000, NULL);

#define doActivate(n) \
	switch (n) {
		\
	case 0: \
			doBlackout(); \
			break; \
	case 1: \
			doLock(); \
			break; \
	case 2: \
			doExtraBall(); \
			break; \
	case 3: \
			doHold(); \
			break; \
	}
		if(ROTATE_ROLLOVER.pressed)
		{
			if (activeActivate != -1)
			{
				doActive(activeActivate);
				activates[activeActivate].state = 0;
				rotateActivates();
				addScore(50, 0);
			}
			else
				addScore(5, 0);
		}
		if(ACTIVATE_TARGET.pressed)
		{
			for (int i = 0; i < 4;i++)
			if (activates[i].state) {
				doActivate(i);
				activates[i].state = 0;
			}
			rotateActivates();
			addScore(50, 0);
		}
	}
	if(mode==PLAY)//blockers
	{
		if(LEFT_BLOCK.pressed)
			lastLeftBlockerOnTime=msElapsed;
		if(RIGHT_BLOCK.pressed)
			lastRightBlockerOnTime=msElapsed;
		if(LEFT_BLOCK.released && lastLeftBlockerOffTime==-1)
		{
			if(lastLeftBlockerOnTime!=-1)
				lastLeftBlockerOffTime=msElapsed;
			lastLeftBlockerOnTime=-1;
			setHeldRelay(LEFT_BLOCK_DISABLE,1);
		}
		if(RIGHT_BLOCK.released && lastRightBlockerOffTime==-1)
		{
			if(lastRightBlockerOnTime!=-1)
				lastRightBlockerOffTime=msElapsed;
			lastRightBlockerOnTime=-1;
			setHeldRelay(RIGHT_BLOCK_DISABLE,1);
		}
		if(lastLeftBlockerOnTime!=-1 && msElapsed-lastLeftBlockerOnTime>max_blocker_on_time)
		{
			setHeldRelay(LEFT_BLOCK_DISABLE,1);
			lastLeftBlockerOffTime=msElapsed;
			lastLeftBlockerOnTime=-1;
		}
		if(lastRightBlockerOnTime!=-1 && msElapsed-lastRightBlockerOnTime>max_blocker_on_time)
		{
			setHeldRelay(RIGHT_BLOCK_DISABLE,1);
			lastRightBlockerOffTime=msElapsed;
			lastRightBlockerOnTime=-1;
		}
		if(lastLeftBlockerOffTime!=-1 && msElapsed-lastLeftBlockerOffTime>blocker_cooldown_time)
		{
			setHeldRelay(LEFT_BLOCK_DISABLE,0);
			lastLeftBlockerOffTime=-1;
			if(LEFT_BLOCK.state)
				lastLeftBlockerOnTime=msElapsed;
		}
		if(lastRightBlockerOffTime!=-1 && msElapsed-lastRightBlockerOffTime>blocker_cooldown_time)
		{
			setHeldRelay(RIGHT_BLOCK_DISABLE,0);
			lastRightBlockerOffTime=-1;
			if(RIGHT_BLOCK.state)
				lastRightBlockerOnTime=msElapsed;
		}
	}
	else
	{
		if (heldRelayState[LEFT_BLOCK_DISABLE] || heldRelayState[RIGHT_BLOCK_DISABLE]) {
			heldRelayState[LEFT_BLOCK_DISABLE] = heldRelayState[RIGHT_BLOCK_DISABLE] = 0;
			updateHeldRelays();
		}
	}/**/
	/*if(mode==PLAY)
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
	}*/

	if(mode==PLAYER_SELECT)
	{
		if(START.pressed)
		{
			startGame();
		}
		/*if(CAB_LEFT.pressed)
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
		}*/
	}
	if(mode==PLAY || mode==DRAIN)
		updateScores();
}
