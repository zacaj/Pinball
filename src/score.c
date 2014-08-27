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


void updateBank(Solenoid *score,Input *zeros,uint16_t target,uint16_t *physical)
{
	//if(target==*physical)
	//	return;
	if(msElapsed-lastScoreFire<100)
		return;

	for(int i=3;i>=0;i--)
	{
		if(getDigit(target,i)!=getDigit(*physical,i) || (getDigit(*physical,i)==0 && zeros[i].state))
		{
			if(&fireSolenoid(&score[i]))
				break;
		}
	}
}

void updateScores()
{
	updateBank(SCORE,SCORE_ZERO,curScore,physicalScore[curPlayer]);
	updateBank(BONUS,BONUS_ZERO,bonus,physicalBonus);
}

void resetScores()
{
#define resetBank(reel,zero) \
	updateSlowInputs(); \
	while(zero[0].state || zero[1].state || zero[2].state || zero[3].state) \
	{ \
		if(msElapsed-lastScoreFire>100) \
		{ \
			int i=0; \
			for(i=0;i<4;i++) \
			{ \
				if(zero[i].state) \
				{ \
					if(fireSolenoid(&reel[i])) \
						break; \
				} \
			} \
		} \
		updateSlowInputs(); \
	}

	for(int i=0;i<4;i++)
	{
		switchPlayerRelay(i);
		resetBank(SCORE,SCORE_ZERO);
		physicalScore[i]=0;
	}
	switchPlayerRelay(-1);
	resetBank(BONUS,BONUS_ZERO);
	physicalBonus=0;
}
