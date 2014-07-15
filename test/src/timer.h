#ifndef TIMER_H
#define TIMER_H

#include <stddef.h>
extern uint32_t (*timerFuncs[])();

void startTimer(uint32_t n);
void setTimer(uint32_t n, uint32_t ms);

void stopTimer(uint32_t n);

void initTimers();

uint8_t callFuncIn(uint32_t (*func)(), uint32_t ms,void *data);
#endif
