/*
 * common.h
 *
 *  Created on: Nov 14, 2012
 *      Author: andrei
 */

#ifndef COMMON_H_
#define COMMON_H_

#include "stm32f30x.h"
#include "stm32f3_discovery.h"
#include "stm32f30x_conf.h"
#include <stdio.h>
#include <stdint.h>

#include "math.h"

#define PI      ((float)3.14159265f)
#define ABS(x)	((x < 0) ? (-x) : x)

extern void _delay_ms(__IO uint32_t ms);
extern void _delay_us(__IO uint32_t us);
extern void Delay(__IO uint32_t nTime);

#endif /* COMMON_H_ */
