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
uint8_t is_restarted_mb=0;
uint8_t activeActivate=0;
uint16_t currentJackpotScore=0;
uint16_t scoreMult=1;
uint8_t nBallInPlay,nBallCaptured,nBallsToFire;
uint32_t lastBallTroughReleaseTime=0;
uint32_t lastBallFireTime=0;
uint32_t lastLeftBlockerOnTime=-1,lastRightBlockerOnTime=-1;
uint32_t lastLeftBlockerOffTime=-1,lastRightBlockerOffTime=-1;
uint8_t currentCapture=0;
enum GameMode {PLAYER_SELECT=0,SHOOT,PLAY,DRAIN};
enum GameMode mode=PLAYER_SELECT;
uint8_t p_captureState[4][3];
uint8_t p_captureStartCount[4][3];
uint16_t p_bonus[4];
uint8_t p_bonusMult[4];
uint8_t p_nLock[4];
uint8_t p_activateStates[4][4];
uint8_t p_redStates[4][4];
uint8_t waitingAcks;
uint32_t p_popsLeft[4];
uint32_t multiballEndTime=0;
uint32_t p_jackpot[4];
uint32_t nPops=0;
uint32_t ballSaveSince=0;

void _log(const char *str,int a,int b,int line);
#define log(str) _log(str,-11,-11,__LINE__)
#define log1(str,a) _log(str,a,-11,__LINE__)
#define log2(str,a,b) _log(str,a,b,__LINE__)
void extraBallReady();
uint32_t turnOffMagnet() {
	setHeldRelay(MAGNET,0);
}
uint8_t flashLed(enum LEDs led, uint32_t period);

void updateExtraBall()
{
	if(msElapsed>ballSaveEndTime && ballSaveSince!=0) {
		ballSaveSince=0;
		//ballSaveEndTime=0;
	}
	if(curScore<ballSaveMinScore || msElapsed<ballSaveEndTime)
		flashLed(SHOOT_AGAIN,180);
	else if(extraBallCount>0)
		setLed(SHOOT_AGAIN,ON);
	else
		setLed(SHOOT_AGAIN,OFF);
}
void startBallSave(uint32_t time)
{
	if(msElapsed>ballSaveEndTime)
		ballSaveSince=msElapsed;
	else if(msElapsed - ballSaveSince>10000)
		return;
	ballSaveEndTime=msElapsed+time;
	updateExtraBall();
}

uint32_t lastScore=0;
typedef struct {
	int line;
	int total;
	int hits;
} ScoreSource;

#define MAX_SCORESOURCE 50
ScoreSource scoreSources[MAX_SCORESOURCE];
int nScoreSource=0;

#define addScore(score,bonus) _addScore(score,bonus,__LINE__)
#define switchHit() lastScore=msElapsed

void _addScore(uint16_t score,uint16_t _bonus, int line)
{
	lastScore=msElapsed;
	if(mode==PLAY)
	{
		_BREAK();
		uint32_t oldScore=curScore;
		curScore+=score*scoreMult;
		bonus+=_bonus*scoreMult;
		
		for(int i=0;i<EB_LEVELS;i++) {
			if(oldScore<ebLevels[i] && curScore>ebLevels[i]) {
				extraBallReady();
				break;
			}
		}
		
		updateExtraBall();
		
		int i;
		for(i=0;i<nScoreSource;i++) {
			if(scoreSources[i].line==line) {
				scoreSources[i].total+=score*scoreMult;
				scoreSources[i].hits++;
				break;
			}
		}
		if(i==nScoreSource) {
			scoreSources[i].line=line;
			scoreSources[i].total=score*scoreMult;
			scoreSources[i].hits=1;
			nScoreSource++;
		}
	}
}

Target redTargets[4]={
		{RED_TARGET_LEFT,0},
		{RED_TARGET_TOP,0},
		{RED_TARGET_BOTTOM,0},
		{RED_TARGET_RIGHT,0},
};
Target lanes[4]={
		{LANE_1,0},
		{LANE_2,0},
		{LANE_3,0},
		{LANE_4,0},
};
Target activates[4]={
		{PURPLE_LED,0},
		{RED_LED,0},//jackpot
		{WHITE_LED,0},//extra
		{YELLOW_LED,0}//start mb
};
int captureNs[]={2,0,1};

void syncCaptureLights();
void rotateActivates();
uint8_t captureState[3];
	
uint32_t ballInLaneUntil=0;
void addBall();
void addBalls(int);
void resetRedTargets();
void endRestartMb() {
	multiballEndTime=0;
	rotateActivates();
}	

void doRestartMb() {
	endRestartMb();
	addBall();
}
void extraBall();
void updateLocks();
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
		if(lockMBMax) {
			log2("add [a] balls to lock mb [b]",nLock,lockMBMax);
			
		}
		log2("start lock mb with [a] locked, scoremult [b]",nLock, scoreMult);
		addBalls(nLock);
		is_restarted_mb=0;
		
		lockMBMax += nLock;
		resetRedTargets();
		nPops=0;
		setHeldRelay(MAGNET, 1);
		callFuncIn(turnOffMagnet, 1500, NULL); 
		break;
	}
}


void threeDown(void *data)
{
	//DropBank *bank=data;
}

void fiveDown(void *data)
{
	DropBank *bank=data;
	if(bank->flashing > 0)
		bank->flashing--;
	for(int i=0;i<4;i++) {
		if(redTargets[i].state==1) 
			redTargets[i].state=2;
	}
	updateLocks();
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
	nBallsToFire++;
	log1("add ball count now [a]",nBallsToFire);
}

void addBalls(int nBall) {
	log2("add [a] balls to [b]",nBall,nBallInPlay);
	nBallsToFire+=nBall;
}

void switchMode(enum GameMode newMode);

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
	switchMode(PLAYER_SELECT);
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
	log1("init drop bank",(int)(bank-dropBanks));
}

void updateDropBank(DropBank *bank)
{
	Input captureIns[]={LEFT_CAPTURE,RIGHT_CAPTURE,TOP_CAPTURE};
	int maxCaptureStartCount=0;
	int atMax=0;
	for(int j=0;j<3;j++)
		if(maxCaptureStartCount<p_captureStartCount[curPlayer][j])
			maxCaptureStartCount=p_captureStartCount[curPlayer][j];
	for(int j=0;j<3;j++)
		if(maxCaptureStartCount==p_captureStartCount[curPlayer][j] || captureIns[j].state)
			atMax++;
	int n=-1;
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
	uint8_t disabled=0;
	if(n>=0 && p_captureStartCount[curPlayer][n]==maxCaptureStartCount && atMax<3)
		disabled=1;
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
				if(disabled) {
					addScore(1,0);
					bank->pressedAt[i]=0;
					continue;
				}
				//log2("drop down",bank,i);
				//log1("flashing: [a]",bank->flashing);
				//BREAK();

				addScore(drop_score*(bank->flashing==i?drop_sequence_mult:1)*(n>=0 && captureState[n]>=2? drop_complete_mult:1),bank->flashing==i?drop_sequence_bonus:0);
				if(bank->flashing==i)//was flashing
				{
					bank->flashing++;
				}
				if (bank->nTarget == 5) {
					if (bank->flashing >= bank->nTarget)
					{
						if(nLock>0)
							doActivate(START_LOCKMB_ACT);
					}
				}
				else if (bank->nTarget == 3) {
					if (bank->flashing == bank->nTarget)
					{

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
							addScore(drop_complete_score*drop_sequence_mult*(captureState[n]>=2? drop_complete_mult:1),0);
							break;
						}
						log2("increment capture [a] to [b]",n,captureState[n]);
						syncCaptureLights();
					}
				}
				if(down==bank->nTarget)//last one
				{
					bank->down(bank);
					rotateActivates();
					addScore(drop_complete_score*(bank->flashing==i?drop_sequence_mult:1)*(n>=0 && captureState[n]>=2? drop_complete_mult:1),drop_complete_bonus);
				}
				if(bank->flashing>=bank->nTarget)
				{
					bank->flashing=0;
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
			//log1("try to reset bank [a]",bank);
			fireSolenoidIn(bank->reset,250);
			bank->resetting=1;
		}
	}
	if(!down)
	{
		bank->resetting=0;
	}


	for(int j=0;j<bank->nTarget;j++) {
		if(disabled) {
			setLed(bank->led[j],OFF);
			break;
		}
		if(n>=0 && captureState[n]>=2) {
			setLed(bank->led[j],ON);
			continue;
		}
		if(j==bank->flashing)
			flashLed(bank->led[j],drop_flash_period);
		else if(j<bank->flashing)
			setLed(bank->led[j],ON);
		else
			setLed(bank->led[j],OFF);
	}
}
uint8_t flashLed(enum LEDs led, uint32_t period)
{
	return setFlash(led,period*4);
}
void startBall();
void startGame()
{
	if(nPlayer==0)
		return;
		
	log1("start game [a] players",nPlayer);
	
	ballNumber=0;
	nScoreSource=0;

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
		for(int j=0;j<4;j++) {
			p_activateStates[i][j]=0;
			p_redStates[i][j]=0;
		}
		for(int j=0;j<3;j++) {
			p_captureState[i][j]=0;
			p_captureStartCount[i][j]=0;
		}
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
	if(!ballInLaneUntil) {
		//BREAK();
		setHeldRelay(BALL_RELEASE,1);
		wait(250);
		setHeldRelay(BALL_RELEASE,0);
		//lastBallTroughReleaseTime=msElapsed;
		ballInLaneUntil=msElapsed+ball_in_lane_wait_time;
	}
	switchMode( SHOOT );
	//BREAK();
	setHeldRelay(BALL_SHOOT_ENABLE,1);
}
void resetRedTargets();
void resetLanes();
	enum LEDs lockLeds[]={LOCK_1,LOCK_2,LOCK_3,LOCK_4};
void updateLocks()
{
	nLock=0;
	uint8_t changed=0;
	for(int i=0;i<4;i++) {
		if(redTargets[i].state==2) {
			nLock++;
			changed|=setLed(lockLeds[i],ON);
			changed|=setLed(redTargets[i].led,ON);
		}
		else if(redTargets[i].state==1) {
			changed|=flashLed(redTargets[i].led,250);
			changed|=flashLed(lockLeds[i],250);
		}
		else {
			changed|=setLed(redTargets[i].led,OFF);
			changed|=setLed(lockLeds[i],OFF);
		}
	}
	
	if(changed) {
		syncLeds(lockLeds[0],lockLeds[1],lockLeds[2],lockLeds[3]);
		syncLeds(redTargets[0].led,redTargets[1].led,redTargets[2].led,redTargets[3].led);
	}
	
	uint8_t oldState=activates[START_LOCKMB_ACT].state;
	if(nLock>0)
		activates[START_LOCKMB_ACT].state=1;
	else
		activates[START_LOCKMB_ACT].state=0;
	if(oldState!=activates[START_LOCKMB_ACT].state)
		rotateActivates();
}
uint32_t lastBallFiredAt=0;
void startBall()
{//
	//BREAK();
	lastBallFiredAt=0;
	for(int i=0;i<nLED;i++)
	{
		setLed(i,OFF);
	}
	nBallInPlay=nBallCaptured=nBallsToFire=0;
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
	is_restarted_mb=0;
	multiballEndTime=0;
	waitingAcks=0;
	setHeldRelay(LEFT_BLOCK_DISABLE,0);
	setHeldRelay(RIGHT_BLOCK_DISABLE,0);
	for(int i=0;i<4;i++) {
		activates[i].state=p_activateStates[curPlayer][i];
		redTargets[i].state=p_redStates[curPlayer][i];
	}
	updateLocks();
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

void extraBallReady() {
	activates[EXTRA_BALL_ACT].state++;
	rotateActivates();
}

void resetRedTargets()
{
	for(int i=0;i<4;i++)
	{
		redTargets[i].state=0;
	}
	updateLocks();
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
			flashLed(led,200);
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
	lastBallFiredAt=0;
	BREAK();
	for(int i=0;i<3;i++) {
		p_captureState[curPlayer][i]=captureState[i]>0?captureState[i]:0;
		if(captureState[i])
			addScore(lost_capture_score,0);
		captureState[i]=0;
	}
	addScore(bonus*bonusMult,0);
	switchMode(DRAIN);
	fireSolenoid(&BALL_SHOOT);
	ballInLaneUntil=0;//hopefully...
	//p_bonus[curPlayer]=bonus*hold/4;
	//p_bonusMult[curPlayer]=bonusMult*hold/4;
	bonus=0;
	
	for(int i=0;i<nLED;i++)
	{
		setFlash(i,3000);
		offsetLed(i,3000*(i%2));
	}
	for(int i=0;i<4;i++)
		setLed(lockLeds[i],i<=ballNumber?ON:OFF);
	if(ballNumber==5)
		setLed(SHOOT_AGAIN,ballNumber==4);
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
			flashLed(activates[i].led,200);
		else
			setLed(activates[i].led,ON);
	}
}

void nextPlayer()
{
	BREAK();
	p_nLock[curPlayer]=nLock>0?nLock:0;
	for(int i=0;i<4;i++) {
		p_activateStates[curPlayer][i]=activates[i].state;
		p_redStates[curPlayer][i]=redTargets[i].state;
	}
	//captureState[currentCapture]--;
	curPlayer++;
	if(curPlayer>=nPlayer)
	{
		curPlayer=0;
		ballNumber++;
		if(ballNumber>=5)
		{
			curPlayer=-1;
			initGame();
		}
	}
	switchPlayerRelay(curPlayer);
}
uint8_t waitingToAutoFireBall=0;
uint32_t ballAckCount=0;
uint32_t lastModeChange=0;
void switchMode(enum GameMode newMode) {
	lastModeChange=msElapsed;
	log2("Mode switch from [a] to [b]",mode,newMode);
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
				setHeldRelay(PLAYFIELD_DISABLE,0);
			
				break;
			case DRAIN:
				setHeldRelay(PLAYFIELD_DISABLE,0);
				break;
		}
	//entering
	if(oldMode!=newMode)
		switch(newMode) {
			case SHOOT:
			
				break;
			case PLAY:

				break;
			case PLAYER_SELECT:
				setHeldRelay(PLAYFIELD_DISABLE,1);

				break;
			case DRAIN:
				setHeldRelay(PLAYFIELD_DISABLE,1);
				break;
		}
}
uint8_t b=0;
void updateGame()
{
	if(LANES[3].rawState && lastBallFiredAt!=0) {
		lastBallFiredAt=0;
		b=1;
	}
	if(mode==SHOOT)
	{
		if(SHOOT_BUTTON.pressed)
		{
			fireSolenoid(&BALL_SHOOT);
			ballInLaneUntil=0;
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
	else if(SHOOT_BUTTON.pressed || (ballInLaneUntil!=0 && ballInLaneUntil<msElapsed) || (msElapsed-lastBallFiredAt>3000 && mode==PLAY && lastBallFiredAt!=0 && lastBallFiredAt<msElapsed)) {
		if(!waitingToAutoFireBall) {
			_BREAK();
			fireSolenoid(&BALL_SHOOT);
			ballInLaneUntil=0;
			lastBallFiredAt=0;
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
			if(!waitingAcks && fireSolenoid(&BALL_ACK)) {
				if(mode==PLAY && msElapsed-lastModeChange>1000)
					waitingAcks=1;
				ballAckCount=0;
			}
		}
		if(mode==PLAY)
		{
			if((BALLS_FULL.rawState && waitingAcks) || (BALL_OUT.pressed && BALLS_FULL.state) || (BALL_OUT.state && msElapsed-BALL_OUT.lastChange>1000 && BALLS_FULL.state && msElapsed-BALLS_FULL.lastChange>1000))//a ball was acked successfully
			{
				if(!(BALL_OUT.pressed && BALLS_FULL.state) && waitingAcks)
					waitingAcks=0;
				log2("ball lost: was [a] captured, [b] in play",nBallCaptured,nBallInPlay);
				_BREAK();
				nBallInPlay--;
				if(nBallInPlay==0)//in single ball
				{
					lockMBMax=0;
					scoreMult=1;
					if(getLed(SHOOT_AGAIN)==FLASHING || msElapsed-ballSaveEndTime<ball_save_fuzz || lastScore<ballSaveEndTime)
					{
						log("saved");
						//addBalls(1);
						startShoot();
					}
					else if(extraBallCount>0)
					{
						log1("shoot again, eb: [a]",extraBallCount);
						extraBallCount--;
						updateExtraBall();
						startShoot();
					}
				}
				else if(nBallInPlay>0) //in multi-ball
				{
					if(getLed(SHOOT_AGAIN)==FLASHING || msElapsed-ballSaveEndTime<mb_save_fuzz || lastScore<ballSaveEndTime)
					{
						log("ball saved mb");
						addBalls(1);
					}
					else {
						if(nBallInPlay==1)
						{
							log("mb over");
							if(!is_restarted_mb) {
								multiballEndTime=msElapsed;
								for(int i=0;i<4;i++) 
									flashLed(activates[i].led,250);
							}
							lockMBMax=0;
							scoreMult=1;
						}	
					}			
				}
				else if(nBallInPlay<1) { //oh oh
					log1("uh oh",nBallInPlay-nBallCaptured);
				}
			}
			if(nBallsToFire==0 && nBallInPlay==0 && !waitingToAutoFireBall && mode==PLAY &&
					msElapsed-LEFT_CAPTURE.lastChange>800 && msElapsed-RIGHT_CAPTURE.lastChange>800
					&& msElapsed-TOP_CAPTURE.lastChange>800)
			{
				log1("ball [a] out", ballNumber);
				startDrain();
			}
		}
	}
	//if(nBallInPlay<0)
	//	nBallInPlay=0;
	if(mode==PLAY)
	{
		if(nBallsToFire>0) //want more balls in play
		{
			if(!waitingToAutoFireBall)
				//start firing a ball if it's been a bit and the max balls aren't out
				if(msElapsed-lastBallFireTime>time_between_ball_fire && nBallInPlay+nBallCaptured<5 && !waitingToAutoFireBall)
				{
					if(!heldRelayState[BALL_RELEASE] && !ballInLaneUntil)
					{
						log("release ball for firing");
						setHeldRelay(BALL_RELEASE,1);
						ballInLaneUntil=msElapsed+ball_in_lane_wait_time;
						lastBallTroughReleaseTime=msElapsed;
						waitingToAutoFireBall=1;
						nBallsToFire--;
						if(nBallsToFire==0) {
							log("balls launched");
							setHeldRelay(MAGNET,0);
						}
					}
				}
		}
	}
	if((msElapsed-lastBallTroughReleaseTime>ball_release_wait_time && lastBallTroughReleaseTime!=0) || lastBallTroughReleaseTime>msElapsed) {
		if(heldRelays[BALL_RELEASE].state) {
			_BREAK();
			setHeldRelay(BALL_RELEASE,0);
			log1("ball release complete, [a] already in play",nBallInPlay);
			if(!waitingToAutoFireBall)
				lastBallTroughReleaseTime=0;
		}
	}
	if(waitingToAutoFireBall && (msElapsed-lastBallTroughReleaseTime>ball_fire_wait_time || lastBallTroughReleaseTime>msElapsed || lastBallTroughReleaseTime==0)) //if ready to shoot, shoot
	{
		_BREAK();
		log1("auto fire, [a] balls already",nBallInPlay);
		lastBallTroughReleaseTime=0;
		lastBallFireTime=msElapsed;
		fireSolenoid(&BALL_SHOOT);
		lastBallFiredAt = msElapsed;
		ballInLaneUntil=0;
		nBallInPlay++;
		startBallSave(consolation_time);
		waitingToAutoFireBall=0;
	}
	{//captures
		Input ins[]={TOP_CAPTURE,LEFT_CAPTURE,RIGHT_CAPTURE};
		Solenoid *ejects[]={&TOP_CAPTURE_EJECT,&LEFT_CAPTURE_EJECT,&RIGHT_CAPTURE_EJECT};
		int *ns=captureNs;
		static uint32_t lastCaptureEjectTime=0;
		for(int i=0;i<3;i++) {
			Input in=ins[i];
			Solenoid* eject=ejects[i];
			int n=ns[i];

			if(mode==PLAY) {
				if(in.pressed)
				{
					//log2("Ball in capture [a], state [b]",n,captureState[n]);
					addScore(capture_score,0);
					nBallInPlay--;
					nBallCaptured++;
					if(captureState[n]==1 || (captureState[n]>=2 && nBallCaptured==3))
					{
						if(captureState[n]==1) {
							p_captureStartCount[curPlayer][n]++;
						}
						else {
							int maxCaptureCount=0;
							for(int j=0;j<3;j++)
								if(maxCaptureCount<p_captureStartCount[curPlayer][j])
									maxCaptureCount=p_captureStartCount[curPlayer][j];
							for(int j=0;j<3;j++)
								p_captureStartCount[curPlayer][j]=maxCaptureCount;
						}
						log1("start capture mb with [a] locked", lockMBMax);
						addBalls(1 + (i==1));
						scoreMult=nBallCaptured+1;
						for(int j=0;j<3;j++)
						{
							if(ins[j].state)
								captureState[ns[j]]=0;
						}
						is_restarted_mb=0;
						syncCaptureLights();
					}
					else if(captureState[n]>=2) {
						is_restarted_mb=0;
						addBalls(1 + (i==1));
						log1("captured, want [a] balls", nBallsToFire);
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
			}
			if(in .state && msElapsed-in.lastChange>250 && (msElapsed-lastCaptureEjectTime>700 || lastCaptureEjectTime==0))
			{
				switch(captureState[n])
				{
				case 0:
				case 1:
					if(fireSolenoid(eject)) {
						log1("ejecting capture from [a]",n);
						lastCaptureEjectTime=msElapsed;
					}
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
			if(RED_TARGET[i].pressed && msElapsed-RED_TARGET[i].lastRawOff>400)
			{
				if(redTargets[i].state==1) {
					addScore(red_target_miss_score,0);
					redTargets[i].state=0;
				}
				else if(redTargets[i].state==0) {
					addScore(red_target_hit_score, red_target_hit_bonus);
					redTargets[i].state=1;
				}
				else if(redTargets[i].state==2) {
					addScore(red_target_complete_score, red_target_complete_bonus);
				}
				updateLocks();
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
						//log1("bonus up to [a]",bonusMult+1);
						resetLanes();
						bonusMult++;
						if(bonusMult%bonus_mult_extra_ball_divisor==0)
							extraBallReady();
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
			/*if(multiballEndTime) {
				doRestartMb();
				is_restarted_mb=1;
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
			}*/
			addScore(10,0);
		}
		if(ACTIVATE_TARGET.pressed)
		{
			if(multiballEndTime) {
				doRestartMb();
			}
			else {
				/*log("multi activate!");
				for (int i = 0; i < 4;i++)
				if (activates[i].state) {
					doActivate(i);
					if(activates[i].state>0)
						activates[i].state--;
				}
				rotateActivates();
				addScore(50, 0);*/
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
	if(mode==PLAY)
	{
		if(BUMPER.pressed)
		{
			/*captureState[currentCapture]--;
			currentCapture++;
			if(currentCapture>2)
				currentCapture=0;
			captureState[currentCapture]++;
			syncCaptureLights();*/
			addScore(0,0);
		}
	}

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
	if(mode==PLAY) {
		syncCaptureLights();
		updateExtraBall();
	}
}

typedef struct {
	uint32_t time;
	const char *str;
	int a;
	int b;
	uint32_t line;
	int nBallInPlay,nBallCaptured,nLock,lockMBMax,scoreMult;
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
	logs[i].nBallInPlay=nBallInPlay;
	logs[i].nBallCaptured=nBallCaptured;
	logs[i].nLock=nLock;
	logs[i].lockMBMax=lockMBMax;
	logs[i].scoreMult=scoreMult;
}
