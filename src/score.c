/*
 * score.c
 *
 *  Created on: Aug 26, 2014
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

uint32_t lastScoreFire=0;
extern uint8_t bonusMult;

void initScores()
{

}

int getDigit(unsigned int number, int digit) {
	 for(int i=0; i<digit-1; i++) { number /= 10; }
	 return number % 10;
}


void updateBank(Solenoid *score,Input *zeros,uint16_t target,uint16_t *physical,uint8_t invert)
{
	//if(target==*physical)
	//	return;
	if(msElapsed-lastScoreFire<50)
		return;

	int ten=1000;
		for(int i=3;i>=0;i--)
		{
			int targetDigit=getDigit(target,i+1);
			int physicalDigit=getDigit(*physical,i+1);

			//if this reel has 'settled'
			if(
				zeros[i].state==zeros[i].rawState
				&&
				msElapsed-score[i].lastFired+10>score[i].offTime+score[i].onTime
				)
			{
				if(!(zeros[i].state^invert) && physicalDigit!=0) //if the reel is zero
					*physical-=(physicalDigit-1)*ten; //then we know what the physical digit is

				//if what we think the physical digit is doesn't match what we want it to be
				//or we know we're wrong about what the physical digit is
				if(targetDigit!=physicalDigit || (physicalDigit!=0) != (zeros[i].state^invert) && msElapsed-zeros[i].lastOff>100)
				{
					if(fireSolenoid(&score[i]))
					{
						_BREAK();
						//if we didn't think we were wrong about the physical digit
						if(targetDigit!=physicalDigit) {
							if(physicalDigit==9)//update what we think the physical digit is
								*physical-=ten*9;
							else
								*physical+=ten;
						}
						lastScoreFire=msElapsed;
						break;
					}
				}
			}
			ten/=10;
		}
}

void resetBank(Solenoid *reel,Input *zero, uint8_t invert) {
	wait(100);
	updateIOs();
	while( (invert^ zero[0].state) ||  (invert^zero[1].state) ||  (invert^ zero[2].state) ||  (invert^ zero[3].state)
			|| zero[0].state!=zero[0].rawState || zero[1].state!=zero[1].rawState
			|| zero[2].state!=zero[2].rawState || zero[3].state!=zero[3].rawState
			|| msElapsed-lastScoreFire<200)
	{
		if(msElapsed-lastScoreFire>50)
		{
			int i=0;
			for(i=0;i<4;i++)
			{
				if( invert^ zero[i].state && zero[i].state==zero[i].rawState)
				{
					if(fireSolenoid(&reel[i]))
					{
						_BREAK();
						//wait(100);
						lastScoreFire=msElapsed;
						break;
					}
				}
			}
		}
		updateIOs();
	}
}

void updateScores()
{
	updateBank(SCORE,SCORE_ZERO,curScore,&physicalScore[curPlayer],0);
	updateBank(BONUS,BONUS_ZERO,bonus*10+bonusMult%10,&physicalBonus,1);
}

void resetScores()
{
	resetBank(BONUS,BONUS_ZERO,1);
	physicalBonus=0;
	for(int i=0;i<nPlayer;i++)
	{
		switchPlayerRelay(i);
		wait(400);
		resetBank(SCORE,SCORE_ZERO,0);
		physicalScore[i]=0;
	}
	switchPlayerRelay(-1);
	wait(100);
	updateIOs();
}
