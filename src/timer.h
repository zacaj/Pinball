#ifndef TIMER_H
#define TIMER_H

#include <stddef.h>
extern uint32_t (*timerFuncs[])(void*);
extern uint32_t msTicks,msElapsed;

void startTimer(uint32_t n);
void setTimer(uint32_t n, uint32_t ms);
void setTimerCustom(uint32_t n, uint32_t prescalar, uint32_t period);

void stopTimer(uint32_t n);

void initTimers();

uint8_t callFuncIn(uint32_t (*func)(void*), uint32_t ms,void *data);
uint8_t callFuncInCustom(uint32_t (*func)(void*), uint32_t prescalar, uint32_t period,void *data);
void callFuncIn_s(uint32_t (*func)(void*), uint32_t ms, void* data);
void wait(uint32_t ms);
extern float fast_cossin_table[];
#endif
