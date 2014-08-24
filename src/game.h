/*
 * game.h
 *
 *  Created on: Aug 23, 2014
 *      Author: zacaj
 */

#ifndef GAME_H_
#define GAME_H_

#define MAX_PLAYER 4

static const uint16_t capture_score=10;
static const uint16_t drop_score=1;
static const uint16_t drop_sequence_mult=5;
static const uint16_t drop_complete_score=5;
static const uint16_t drop_complete_bonus=1;
static const uint16_t drop_sequence_bonus=1;
static const uint16_t red_target_miss_score=1;
static const uint16_t red_target_hit_score=1;

typedef struct
{
	uint8_t nTarget;
	Input* in;
	enum LEDs led[5];
	uint8_t flashing;
	Solenoid *reset;
	void (*down)(void *);
} DropBank;

typedef struct
{
	enum LEDs led;
	uint8_t state;
} RedTarget;


extern DropBank dropBanks[4];

extern uint16_t playerScore[MAX_PLAYER];
extern uint16_t bonus;
extern uint8_t curPlayer;
#define curScore playerScore[curPlayer]
extern uint8_t ballNumber,nPlayer;


void updateGame();
void initGame();


#endif /* GAME_H_ */
