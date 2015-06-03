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

#define EB_LEVELS 5
uint32_t ebLevels[]={1000, 5000, 10000, 20000, 30000};

uint16_t playerScore[];
uint8_t curPlayer=0;
uint16_t bonus=0;
uint16_t physicalBonus=0;
uint16_t physicalScore[4];
signed int ballNumber=1,nPlayer=0;
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
uint8_t waitingAcks;
uint32_t p_popsLeft[4];
uint32_t multiballEndTime=0;
uint32_t p_jackpot[4];
uint32_t nPops=0;

void _log(const char *str,int a,int b,int line);
#define log(str) _log(str,-11,-11,__LINE__)
#define log1(str,a) _log(str,a,-11,__LINE__)
#define log2(str,a,b) _log(str,a,b,__LINE__)

uint32_t turnOffMagnet() {
	setHeldRelay(MAGNET,0);
}
void updateExtraBall()
{
	if(curScore<ballSaveMinScore || msElapsed<ballSaveEndTime)
		flashLed(SHOOT_AGAIN,200);
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
		uint32_t oldScore=curScore;
		curScore+=score*scoreMult;
		bonus+=_bonus*scoreMult;
		
		for(int i=0;i<EB_LEVELS;i++) {
			if(oldScore<ebLevels[i] && curScore>ebLevels[i]) {
				extraBallCount++;
				break;
			}
		}
		
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
		{BLACKOUT_LIGHT,0},
		{START_LOCK_LIGHT,0},
		{EXTRA_BALL_LIGHT,0},
		{BONUS_HOLD_INCREMENT,0}
};

void syncCaptureLights();
void rotateActivates();
uint8_t captureState[3];
	
void addBall();
void endRestartMb() {
	multiballEndTime=0;
	rotateActivates();
}	

void doRestartMb() {
	endRestartMb();
	addBall();
}
	
void doActivate(int n) {
	switch (n) {
	case BLACKOUT_ACT:
		setHeldRelay(MAGNET, 1);
		callFuncIn(turnOffMagnet, 1000, NULL);
		break;
	case JACKPOT_ACT:
		if(lockMBMax) {
			log1("jackpot!!!",currentJackpotScore);
			addScore(currentJackpotScore, jackpot_bonus);
			currentJackpotScore *= jackpot_score_mult;
			nPops=0;
			p_popsLeft[curPlayer]*=jackpot_score_mult;
		}
		setHeldRelay(MAGNET, 1);
		callFuncIn(turnOffMagnet, 1500, NULL); 
		break;
	case EXTRA_BALL_ACT:
		resetRedTargets();
		extraBall();
		break;
	case START_LOCKMB_ACT:
		if(!lockMBMax) {
			log2("start lock mb with [a] locked, scoremult [b]",nLock, scoreMult);
			nMinTargetBallInPlay = nBallInPlay + nLock;
			lockMBMax = nLock;
			nPops=0;
		}
		setHeldRelay(MAGNET, 1);
		callFuncIn(turnOffMagnet, 1500, NULL); 
		break;
	}
}


void threeDown(void *data)
{
	DropBank *bank=data;
}

void fiveDown(void *data)
{
	DropBank *bank=data;
	setLocks(nLock + 1,1);
}


DropBank dropBanks[4]={
		{3,DROP_TARGET[0],{LEFT_1,LEFT_2,LEFT_3,0,0},0,&LEFT_DROP_RESET,0,{0,0,0,0,0},threeDown},
		{3,DROP_TARGET[1],{RIGHT_1,RIGHT_2,RIGHT_3,0,0},0,&RIGHT_DROP_RESET,0,{0,0,0,0,0},threeDown},
		{3,DROP_TARGET[2],{TOP_1,TOP_2,TOP_3,0,0},0,&TOP_DROP_RESET,0,{0,0,0,0,0},threeDown},
		{5,FIVE_TARGET,{FIVE_1,FIVE_2,FIVE_3,FIVE_4,FIVE_5},0,&FIVE_DROP_RESET,0,{0,0,0,0,0},fiveDown},
};

void addBall() {
	if(lockMBMax>0) {
		lockMBMax++;
		log1("add a ball to lock now [a]",lockMBMax);
	}
	if(nMinTargetBallInPlay==0) 
		nMinTargetBallInPlay=nBallInPlay+1;
	else 
		nMinTargetBallInPlay++;
	log1("add ball min now [a]",nMinTargetBallInPlay);
}

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
		p_popsLeft[i]=initial_pops_count;
	}

	for(int i=0;i<nLED;i++)
	{
		//setPWMFunc(i,pwmFunc,i);
		//setPWM(i,255);
		//setLed(i,ON);
		setFlash(i,rand()%1500+2000);
		offsetLed(i,3000*(i%2));
		//setLed(i,FLASHING);
	}
	nPlayer=1;
	log("init game");
}

void initDropBank(DropBank *bank) {
	int down=0;
	bank->flashing = 0;
	do
	{
		updateIOs();
		down=0;
		for(int i=0;i<bank->nTarget;i++) {
			if(bank->in[i].rawState)
				down++;
		}
		if (down)
		{
			BREAK();
			bank->resetting = 1;
			fireSolenoid(bank->reset);
			wait(200);
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
	log1("init drop bank",bank);
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
				bank->pressedAt[i]=msElapsed;
	
			if((bank->in[i].pressed && bank->flashing==i) || 
				(bank->pressedAt[i]!=0 && msElapsed-bank->pressedAt[i]>100))
			{
				log2("drop down",bank,i);
				log1("flashing: [a]",bank->flashing);
				//BREAK();
				addScore(drop_score*(bank->flashing==i?drop_sequence_mult:1),bank->flashing==i?drop_sequence_bonus:0);
				if(bank->flashing==i)//was flashing
				{
					setLed(bank->led[bank->flashing],ON);
					bank->flashing++;
					flashLed(bank->led[bank->flashing],drop_flash_period);
				}
				if (bank->nTarget == 5) {
					if (bank->flashing == bank->nTarget)
					{
						addBall();
					}
					else if(bank->flashing > 0)
						bank->flashing--;
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
							addScore(drop_complete_score*drop_sequence_mult,0);
							break;
						}
						log2("increment capture [a] to [b]",n,captureState[n]);
						syncCaptureLights();
					}
				}
				if(down==bank->nTarget)//last one
				{
					rotateActivates();
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
				bank->pressedAt[i]=0;
			}
		}
	if(down==bank->nTarget)
	{
		int i=0;
		for(i=0;i<bank->nTarget;i++)
			if(bank->pressedAt[i]!=0)
				break;
		if(i==bank->nTarget) {
			log1("try to reset bank [a]",bank);
			fireSolenoidIn(bank->reset,250);
			bank->resetting=1;
		}
	}
	if(!down)
	{
		bank->resetting=0;
	}
}
void flashLed(enum LEDs led, uint32_t period)
{
	setFlash(led,period*4);
}
void startGame()
{
	if(nPlayer==0)
		return;
		
	log1("start game [a] players",nPlayer);
	
	ballNumber=0;

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
		p_popsLeft[i]=initial_pops_count;
		p_nLock[i]=0;
		for(int j=0;j<4;j++)
			p_activateStates[i][j]=0;
		for(int j=0;j<3;j++)
			p_captureState[i][j]=0;
	}
	bonus=0;
	bonusMult=0;
	setHeldRelay(PLAYFIELD_DISABLE,0);
	resetScores();
	wait(500);
	for(int i=3;i>=0;i--)
		initDropBank(&dropBanks[i]);
	curPlayer=0;
	switchPlayerRelay(curPlayer);
	startBall();
}

void startShoot()
{
	ballSaveMinScore=curScore;
	startBallSave(100);
	//BREAK();
	setHeldRelay(BALL_RELEASE,1);
	wait(250);
	setHeldRelay(BALL_RELEASE,0);
	lastBallTroughReleaseTime=msElapsed;
	switchMode(SHOOT);
	//BREAK();
	setHeldRelay(BALL_SHOOT_ENABLE,1);
}
void resetRedTargets();
void resetLanes();
void setLocks(int lock,uint8_t refreshLights)
{
	nLock=lock;
	if(nLock>4) {
		nLock=4;
		addBall();
	}
	enum LEDs lockLeds[]={LOCK_1,LOCK_2,LOCK_3,LOCK_4};
	for(int i=1;i<=4;i++)
		setLed(lockLeds[i-1],i<=nLock?ON:OFF);
	if(refreshLights && !lockMBMax) {
		uint8_t oldState=activates[START_LOCKMB_ACT].state;
		if(nLock>0)
			activates[START_LOCKMB_ACT].state=1;
		else
			activates[START_LOCKMB_ACT].state=0;
		if(oldState!=activates[START_LOCKMB_ACT].state)
			rotateActivates();
	}
}
void startBall()
{//
	//BREAK();
	for(int i=0;i<nLED;i++)
	{
		setLed(i,OFF);
	}
	nBallInPlay=nBallCaptured=nMinTargetBallInPlay=0;
	for(int i=0;i<3;i++)
	{
		captureState[i]=p_captureState[curPlayer][i];
	}
	log2("start ball [a] player [b]",ballNumber,curPlayer);
	p_popsLeft[curPlayer]=initial_pops_count;
	syncCaptureLights();
	for (int i = 0; i < 4; i++)
		initDropBank(&dropBanks[i]);
	resetRedTargets();
	resetLanes();
	currentJackpotScore=jackpot_score;
	scoreMult=1;
	nPops=0;
	bonusMult=p_bonusMult[curPlayer];
	leftPopScore1=1;
	leftPopScore2=1;
	rightPopScore1=1;
	rightPopScore2=1;
	lastLeftBlockerOnTime=-1;
	lastRightBlockerOnTime=-1;
	lastLeftBlockerOffTime=-1;
	lastRightBlockerOffTime=-1;
	multiballEndTime=0;
	waitingAcks=0;
	setHeldRelay(LEFT_BLOCK_DISABLE,0);
	setHeldRelay(RIGHT_BLOCK_DISABLE,0);
	setLocks(p_nLock[curPlayer],1);
	for(int i=0;i<4;i++)
		activates[i].state=p_activateStates[curPlayer][i];
	rotateActivates();
	bonus=p_bonus[curPlayer];
	//BREAK();
	startShoot();
}

void extraBall()
{
	extraBallCount++;
	log1("extra ball, now [a]",extraBallCount);
	if(extraBallCount>max_extra_balls_per_ball)
	{
		log("max extra balls");
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
	flashLed(redTargets[i].led,300);
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
	enum LEDs leds[]={LEFT_CAPTURE_LIGHT,RIGHT_CAPTURE_LIGHT,TOP_CAPTURE_LIGHT};
	for(int i=0;i<3;i++) {
		int n=i;
		enum LEDs led=leds[i];
		switch(captureState[n])
		{
		case 0:
			setLed(led,OFF);
			break;
		case 1:
			flashLed(led,400);
			break;
		case 2:
		case 3:
			setLed(led,ON);
			break;
		}
	}
}
void switchPlayerRelay(int n)
{
	//BREAK();
	for(int i=0;i<4;i++)
		heldRelayState[PLAYER_ENABLE[i]]=0;
	if(n!=-1)
		heldRelayState[PLAYER_ENABLE[n]]=1;
	updateHeldRelays();
}

void startDrain()
{
	BREAK();
	for(int i=0;i<3;i++) {
		p_captureState[curPlayer][i]=captureState[i]>0?captureState[i]:0;
		captureState[i]=0;
	}
	addScore(bonus*bonusMult,0);
	switchMode(DRAIN);
	fireSolenoid(&BALL_SHOOT);
	//p_bonus[curPlayer]=bonus*hold/4;
	//p_bonusMult[curPlayer]=bonusMult*hold/4;
	bonus=0;
	
	for(int i=0;i<nLED;i++)
	{
		setFlash(i,3000);
		offsetLed(i,3000*(i%2));
	}
	bonusMult=0;
}

void rotateActivates()
{
	int i = 0;
	do {
		activeActivate++;
		activeActivate%=4;
		i++;
		if (activates[activeActivate].state)
			break;
	} while (i < 4);
	if (!activates[activeActivate].state)
		activeActivate = -1;
	for(int i=0;i<4;i++)
	{
		if(activates[i].state==0)
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
	p_nLock[curPlayer]=nLock>0?nLock:0;
	for(int i=0;i<4;i++)
		p_activateStates[curPlayer][i]=activates[i].state;
	//captureState[currentCapture]--;
	curPlayer++;
	if(curPlayer>=nPlayer)
	{
		curPlayer=0;
		ballNumber++;
		if(ballNumber>=5)
		{
			curPlayer=-1;
			switchMode(PLAYER_SELECT);
		}
	}
	switchPlayerRelay(curPlayer);
}
uint8_t waitingToAutoFireBall=0;
uint32_t ballAckCount=0;
void switchMode(enum GameMode newMode) {
	log2("Mode switch",mode,newMode);
	enum GameMode oldMode=mode;
	mode=newMode;
	//leaving
	if(oldMode!=newMode) 
		switch(oldMode) {
			case SHOOT:
			
				break;
			case PLAY:
			
				break;
			case PLAYER_SELECT:
			
				break;
			case DRAIN:
				setHeldRelay(PLAYFIELD_DISABLE,0);
				break;
		}
	//leaving
	if(oldMode!=newMode)
		switch(newMode) {
			case SHOOT:
			
				break;
			case PLAY:

				break;
			case PLAYER_SELECT:

				break;
			case DRAIN:
				setHeldRelay(PLAYFIELD_DISABLE,1);
				break;
		}
}
void updateGame()
{
	if(mode==SHOOT)
	{
		if(SHOOT_BUTTON.pressed)
		{
			fireSolenoid(&BALL_SHOOT);
			setHeldRelay(BALL_SHOOT_ENABLE,0);
			nBallInPlay++;
			log("fire");
		}
		if(LANES[3].pressed)
		{
			log("begin ball");
			switchMode(PLAY);
			startBallSave(consolation_time); 
			ballSaveMinScore=curScore+consolation_score;
			updateExtraBall();
			LANES[3].pressed=0;
		}
		if(BALL_OUT.pressed)
		{ 
			log("missed");
			startShoot();
		}
	}
	if(mode==DRAIN)
	{
		if(BALLS_FULL.state && BALL_OUT.state)
		{
			log("finish drain");
			if(ballNumber==-1)
				startGame();
			else if(physicalScore[curPlayer]==curScore && physicalBonus==0) {
				nextPlayer();
				if(mode!=PLAYER_SELECT)
					startBall();
			}
		}
	}
	{//ack
		if(BALL_OUT.state && !BALLS_FULL.state) {
			ballAckCount++;
		}
		else
			ballAckCount=0;
		if(ballAckCount>1500) {
			if(fireSolenoid(&BALL_ACK)) {
				if(mode==PLAY)
					waitingAcks=1;
				ballAckCount=0;
			}
		}
		if(mode==PLAY)
		{
			if((BALLS_FULL.rawState && waitingAcks>0) || (BALL_OUT.pressed && BALLS_FULL.state))//a ball was acked successfully
			{
				if(!(BALL_OUT.pressed && BALLS_FULL.state) && waitingAcks>0)
					waitingAcks=0;
				log2("ball lost: was [a] captured, [b] in play",nBallCaptured,nBallInPlay);
				_BREAK();
				nBallInPlay--;
				if(nBallInPlay==0)//in single ball
				{
					lockMBMax=0;
					scoreMult=1;
					if(getLed(SHOOT_AGAIN)==FLASHING)
					{
						log("saved");
						startShoot();
					}
					else if(extraBallCount>0 && nBallCaptured==0)
					{
						log1("shoot again, eb: [a]",extraBallCount);
						extraBallCount--;
						updateExtraBall();
						startShoot();
					}
				}
				else if(nBallInPlay>0) //in multi-ball
				{
					if(nBallInPlay<lockMBMax && nLock>0)
					{
						log("ball lost, shoot from lock");
						nMinTargetBallInPlay=nBallInPlay+1;
					}
					else if(getLed(SHOOT_AGAIN)==FLASHING)
					{
						log("ball saved mb");
						nMinTargetBallInPlay=nBallInPlay+1;
					}
					else {
						if(nBallInPlay==1)
						{
							log("mb over");
							multiballEndTime=msElapsed;
							for(int i=0;i<4;i++) 
								flashLed(activates[i].led,300);
							lockMBMax=0;
						}
						if(scoreMult!=1 && scoreMult>nBallInPlay)
							scoreMult=nBallInPlay;			
					}			
				}
				else if(nBallInPlay<1) { //oh oh
					log1("uh oh",nBallInPlay-nBallCaptured);
				}
			}
			if(nMinTargetBallInPlay==0 && nBallInPlay==0 && !waitingToAutoFireBall && mode==PLAY)
			{
				if(nBallCaptured>0) {
					//log("ejecting captures");
					for(int i=0;i<3;i++)
					{
						captureState[i]=0;
					}
					syncCaptureLights();
				}
				else {
					log1("ball [a] out", ballNumber);
					startDrain();
				}
			}
		}
	}
	if(nBallInPlay<0)
		nBallInPlay=0;
	if(mode==PLAY)
	{
		if(nMinTargetBallInPlay>nBallInPlay) //want more balls in play
		{
			if(nMinTargetBallInPlay-nBallInPlay>0) //if
			{
				if(!waitingToAutoFireBall)
					goto startFireBall;
			}
			
			goto end;
		startFireBall:
			//start firing a ball if it's been a bit and the max balls aren't out
			if(msElapsed-lastBallTroughReleaseTime>500 && nBallInPlay+nBallCaptured<5 && !waitingToAutoFireBall)
			{
				if(!heldRelayState[BALL_RELEASE])
				{
					log("release ball for firing");
					setHeldRelay(BALL_RELEASE,1);
					lastBallTroughReleaseTime=msElapsed;
					waitingToAutoFireBall=1;
				}
			}
		}
		else if(nMinTargetBallInPlay)//done firing
		{
			log1("[a] requested balls launched",nMinTargetBallInPlay);
			nMinTargetBallInPlay=0;
			setHeldRelay(MAGNET,0);
		}
	end:;
	}
	if(waitingToAutoFireBall && (msElapsed-lastBallTroughReleaseTime>ball_release_wait_time || lastBallTroughReleaseTime==0 || lastBallTroughReleaseTime>msElapsed)) //if ready to shoot, shoot
	{
		log1("auto fire, [a] balls already",nBallInPlay);
		setHeldRelay(BALL_RELEASE,0);
		lastBallTroughReleaseTime=msElapsed;
		fireSolenoid(&BALL_SHOOT);
		nBallInPlay++;
		startBallSave(consolation_time);
		if(lockMBMax)
			setLocks(nLock-1,1);
		waitingToAutoFireBall=0;
		goto end;
	}
	{//captures
		Input ins[]={TOP_CAPTURE,LEFT_CAPTURE,RIGHT_CAPTURE};
		Solenoid *ejects[]={&TOP_CAPTURE_EJECT,&LEFT_CAPTURE_EJECT,&RIGHT_CAPTURE_EJECT};
		static uint32_t lastCaptureEjectTime=0;
		int ns[]={2,0,1};
		for(int i=0;i<3;i++) {
			Input in=ins[i];
			Solenoid* eject=ejects[i];
			int n=ns[i];
			if(in.pressed)
			{
				log2("Ball in capture [a], state [b]",n,captureState[n]);
				addScore(capture_score,0);
				nBallInPlay--;
				nBallCaptured++;
				if(captureState[n]==1 || (captureState[n]>=2 && nBallCaptured==3))
				{
					log1("start capture mb with [a] locked", lockMBMax);
					nMinTargetBallInPlay=nBallInPlay+1;
					scoreMult=nBallCaptured+nBallInPlay+1;
					for(int i=0;i<3;i++)
					{
						captureState[i]=0;
					}
					syncCaptureLights();
				}
				else if(captureState[n]>=2) {
					log("locked");
					nMinTargetBallInPlay=nBallInPlay+1;
				}
				log2("[a] captured, [b] now in play",nBallCaptured,nBallInPlay);
			}
			if(in.released)
			{
				log1("ball left capture [a]",n);
				nBallCaptured--;
				nBallInPlay++;
				log2("[a] captured, [b] now in play",nBallCaptured,nBallInPlay);
			}
			if(in .state && msElapsed-in.lastChange>250 && (msElapsed-lastCaptureEjectTime>700 || lastCaptureEjectTime==0))
			{
				switch(captureState[n])
				{
				case 0:
				case 1:
					if(fireSolenoid(eject))
						log1("ejecting capture from [a]",n);
					break;
				case 2:
					break;
				}
			}
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
				BREAK();
				log2("red [a] hit, state [b]",i,redTargets[i].state);
				switch(redTargets[i].state)
				{
				case 0: // off
				case 2://.5
				case 4://on
					addScore(red_target_miss_score,0);
					sendCommand(1);
					break;
				case 1://fast flashing
				case 3:
					{
						sendCommand(0);
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
							activates[EXTRA_BALL_ACT].state++;
							rotateActivates();
						}
						else if(on==4) {
							int j=rand()%4;
							log1("flash1 [a]",j);
							redTargets[j].state++;
							flashLed(redTargets[j].led,300);
						}
						else
						{
							int j;
							do
							{
								j=rand()%4;
							} while(redTargets[j].state>=redTargets[i].state);
							redTargets[j].state++;
							flashLed(redTargets[j].led,300);
							log1("flash2 [a]",j);
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
						log1("bonus up to [a]",bonusMult+1);
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
		if(RIGHT_FLIPPER.pressed || RIGHT_BLOCK.pressed)
		{
			uint8_t temp=lanes[0].state;
			lanes[0].state=lanes[3].state;
			lanes[3].state=lanes[2].state;
			lanes[2].state=lanes[1].state;
			lanes[1].state=temp;
			for(int i=0;i<4;i++)
				setLed(lanes[i].led,lanes[i].state?ON:OFF);
		}
		if(LEFT_FLIPPER.pressed || LEFT_BLOCK.pressed)
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
			
			if(lockMBMax && nPops<p_popsLeft[curPlayer]) {
				nPops++;
				log2("[a] pops, up to [b]",nPops,p_popsLeft[curPlayer]);
				if(nPops==p_popsLeft[curPlayer]) {
					log("activate jackpot");
					activates[JACKPOT_ACT].state=1;
					rotateActivates();
				}
			}
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
		if(ROTATE_ROLLOVER.pressed)
		{
			if(multiballEndTime) {
				doRestartMb();
			}
			else {
				log2("single activate? [a]",activeActivate,activates[activeActivate].state);
				if (activeActivate != -1)
				{
					doActivate(activeActivate);
					if(activates[activeActivate].state>0);
						activates[activeActivate].state--;
					rotateActivates();
					addScore(50, 0);
				}
				else
					addScore(5, 0);
			}
		}
		if(ACTIVATE_TARGET.pressed)
		{
			if(multiballEndTime) {
				doRestartMb();
			}
			else {
				log("multi activate!");
				for (int i = 0; i < 4;i++)
				if (activates[i].state) {
					doActivate(i);
					if(activates[i].state>0);
						activates[i].state--;
				}
				rotateActivates();
				addScore(50, 0);
			}
		}
	}
	if(mode==PLAY) {
		if(msElapsed-multiballEndTime>restart_mb_time && multiballEndTime!=0) {
			endRestartMb();
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
			if(lastLeftBlockerOnTime!=-1 && msElapsed-lastLeftBlockerOnTime>50) {
				lastLeftBlockerOffTime=msElapsed;
				setHeldRelay(LEFT_BLOCK_DISABLE,1);
				lastLeftBlockerOnTime=-1;
			}
		}
		if(RIGHT_BLOCK.released && lastRightBlockerOffTime==-1)
		{
			if(lastRightBlockerOnTime!=-1 && msElapsed-lastRightBlockerOnTime>50) {
				lastRightBlockerOffTime=msElapsed;
				setHeldRelay(RIGHT_BLOCK_DISABLE,1);
				lastRightBlockerOnTime=-1;
			}
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
			ballNumber=-1;
			startDrain();
		}
		if(CAB_LEFT.pressed)
		{
			if(nPlayer>1)
				nPlayer--;
			switchPlayerRelay(nPlayer-1);
		}
		if(CAB_RIGHT.pressed)
		{
			if(nPlayer<MAX_PLAYER)
				nPlayer++;
			switchPlayerRelay(nPlayer-1);
		}
	}
	if(mode==PLAY || mode==DRAIN)
		updateScores();
	if(mode==PLAY)
		updateExtraBall();
}

typedef struct {
	uint32_t time;
	const char *str;
	int a;
	int b;
	uint32_t line;
} Log;

#define MAX_LOGS 200
Log logs[200];
uint32_t nLog=0;

void _log(const char *str,int a,int b,int line) {
	int i=(nLog++)%MAX_LOGS;
	logs[i].time=msElapsed;
	logs[i].str=str;
	logs[i].a=a;
	logs[i].b=b;
	logs[i].line=line;
}
