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
static const uint16_t red_target_hit_score=10;
static const uint16_t red_target_hit_bonus=1;
static const uint16_t red_target_complete_bonus=1;
static const uint16_t lane_hit_score=10;
static const uint16_t lane_miss_score=1;
static const uint16_t lane_complete_bonus=1;
static const uint16_t bonus_mult_extra_ball_divisor=10;
static const uint32_t left_pop_hit_time=500;
static const uint32_t right_pop_hit_time=500;
static const uint8_t max_extra_balls_per_ball=2;
static const uint16_t consolation_score=10;
static const uint32_t consolation_time=5000;
static const uint16_t jackpot_score=500;
static const uint16_t jackpot_score_mult=2;
static const uint16_t jackpot_bonus=10;
static const uint32_t max_blocker_on_time=2500;
static const uint32_t blocker_cooldown_time=1500;

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
} Target;


extern DropBank dropBanks[4];

extern uint16_t playerScore[MAX_PLAYER];
extern uint16_t physicalScore[4];
extern uint16_t bonus;
extern uint16_t physicalBonus;
extern uint8_t curPlayer;
#define curScore playerScore[curPlayer]
extern uint8_t ballNumber,nPlayer;


void updateGame();
void initGame();
void switchPlayerRelay(int n);//-1 for all off


#endif /* GAME_H_ */
