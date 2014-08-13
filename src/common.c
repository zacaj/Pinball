/*
 * common.c
 *
 *  Created on: Nov 14, 2012
 *      Author: andrei
 */


#include "common.h"


__IO uint32_t TimingDelay = 0;


/**
 * @brief  Inserts a delay time.
 * @param  nTime: specifies the delay time length, in 10 ms.
 * @retval None
 */

/**********************************/
extern uint32_t msTicks,msElapsed;
void SysTick_Handler(void)
{
  static uint32_t count = 1000;

  if (count>0)
  {
    count--;
  }
  if (0==count)
  {
    count = 1000;
    //timerFlag = 1;
  }
  msTicks++;
  msElapsed++;
}
