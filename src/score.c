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

/*uint16_t pow[4]={1,10,100,1000};
int getDigit(const unsigned int number, const unsigned int digit) {
    if (digit > 0 && digit <= log10(number)+1) {
       return (int)(number / pow[(digit-1)]) % 10;
    }  else {
        return 0;
    }
}*/
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
		if(targetDigit!=physicalDigit || ((physicalDigit!=0) != (zeros[i].state^invert) && 1))
		{
			if(fireSolenoid(&score[i]))
			{
				_BREAK();
				if(targetDigit!=physicalDigit && !((physicalDigit!=0) != (zeros[i].state^invert))) {
					if(physicalDigit==9)
						*physical-=ten*9;
					else
						*physical+=ten;
				}
				else {
					if(!(zeros[i].state^invert)) //if the reel is zero
						*physical-=(physicalDigit-1)*ten;
				}
				lastScoreFire=msElapsed;
				break;
			}
		}
		ten/=10;
	}
}

void resetBank(Solenoid *reel,Input *zero, uint8_t invert) {
	wait(100);
	updateIOs();
	while( (invert^ zero[0].state) ||  (invert^zero[1].state) ||  (invert^ zero[2].state) ||  (invert^ zero[3].state))
	{
		if(msElapsed-lastScoreFire>50)
		{
			int i=0;
			for(i=0;i<4;i++)
			{
				if( invert^ zero[i].state)
				{
					if(fireSolenoid(&reel[i]))
					{
						_BREAK();
						wait(200);
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
	for(int i=0;i<4-1;i++)
	{
		switchPlayerRelay(i);
		wait(250);
		resetBank(SCORE,SCORE_ZERO,0);
		physicalScore[i]=0;
	}
	switchPlayerRelay(-1);
	wait(100);
	updateIOs();

	physicalScore[1]=0;
	physicalBonus=0;
}
