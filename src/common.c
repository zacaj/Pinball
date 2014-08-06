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
void Delay(__IO uint32_t nTime)
{
	TimingDelay = nTime;

	while(TimingDelay != 0);
}


/**********************************/
void SysTick_Handler(void)
{
	if (TimingDelay != 0x00)
	{
		TimingDelay--;
	}
	//USBConnectTimeOut--;
	//DataReady ++;
}

