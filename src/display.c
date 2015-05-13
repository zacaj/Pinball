/*
 * display.c
 *
 *  Created on: Jan 24, 2015
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

/*
 *     aaa bbb
 *    h1  2  3c
 *    h 1 2 3 c
 *    h  123  c
 *     888 444
 *    g  765  d
 *    g 7 6 5 d
 *    g6  6  5d
 *     fff eee
 */
/*
 * uint16_t 87654321hgfedcba
 */



uint16_t symbol(char c)
{
#undef S
#define S(c,a) case c: return a;
#define a 1<<0
#define b 1<<1
#define c 1<<2
#define d 1<<3
#define e 1<<4
#define f 1<<5
#define g 1<<6
#define h 1<<7
#define sa 1<<0
#define sb 1<<1
#define sc 1<<2
#define sd 1<<3
#define se 1<<4
#define sf 1<<5
#define sg 1<<6
#define sh 1<<7
#define s1 1<<8
#define s2 1<<9
#define s3 1<<10
#define s4 1<<11
#define s5 1<<12
#define s6 1<<13
#define s7 1<<14
#define s8 1<<15

#define L g|h
#define R c|d
#define B f|e
#define T a|b
#define D s8|s4
#define M s2|s6
#define sL g|h
#define sR c|d
#define sB f|e
#define sT a|b
#define sD s8|s4
#define sM s2|s6
	switch(c)
	{
		S(' ',0);
		S('0',a|b|c|d|e|f|g|h);
		S('1',s2|s6);
		S('2',a|b|c|s4|s8|g|f|e);
		S('3',a|b|c|s4|d|e|f);
		S('4',c|d|s8|s4|h);
		S('5',a|b|h|s8|s4|d|e|f);
		S('6',b|a|g|g|f|e|d|s4|s8);
		S('7',a|b|s3|s6);
		S('8',a|b|c|d|e|f|g|D);
		S('9',a|b|c|d|h|s8|s4);
		S('A',L|R|T|D);
		S('B',T|B|R|B|s4);
		S('C',T|L|B);
		S('D',T|B|R|M);
		S('E',L|T|B|D);
		S('F',T|L|s8);
		S('G',T|L|B|d|s4);
		S('H',L|R|D);
		S('I',M|T|B);
		S('J',R|B|g);
		S('K',L|s8|s3|s5);
		S('L',L|B);
		S('M',L|R|s1|s3);
		S('N',L|R|s1|s5);
		S('O',L|R|T|B);
		S('P',L|T|D|c);
		S('Q',T|B|L|R|s5);
		S('R',L|T|D|c|s5);
		S('S',B|T|d|s1|s4);
		S('T',T|M);
		S('U',L|R|B);
		S('V',L|s7|s3);
		S('W',L|R|s5|s7);
		S('X',s1|s3|s5|s7);
		S('Y',s1|s3|s6);
		S('Z',T|B|s3|s7);
		S('+',M|D);
		S('|',M);
		S('-',D);
		S('_',B);
		S('=',B|D);
		S('%',h|d|s3|s7);
		S('(',a|L|f);
		S(')',b|R|e);
		S('/',s3|s7);
		S('\\',s1|s5);
		S('\'',s3);
		S(',',s7);
		S('`',s1);
		S('?',T|s3|s6);
		S('*',s1|s2|s3|s5|s6|s7);
		S('<',s3|s5);
		S('>',s1|s7);
		S('^',s7|s5);
		S('v',s1|s3);
		S('#',T|B|R|L|M|D);
		S('@',T|B|R|L|M|D|s1|s3|s5|s7);
		S('l',L);
		S('i',R);
		S(':',T);
	default:
		BREAK();
	}
#undef S
#undef a
#undef b
#undef c
#undef d
#undef e
#undef f
#undef g
#undef h
#undef T
#undef B
#undef L
#undef R
#undef M
#undef D
}

const IOPin SHIFT_DATA={bU,P0};
const IOPin SHIFT_CLOCK={bU,P0};
const IOPin SHIFT_LATCH={bU,P0};

const IOPin HIGH[]={
					{bC,P3}, {bD, P2}
};

int nDigit=10;
int curDigit=0;

uint16_t digits[10];

void initDisplay()
{
	/*initOutput(SHIFT_DATA);
	setOutDirect(SHIFT_DATA,0);
	initOutput(SHIFT_CLOCK);
	setOutDirect(SHIFT_CLOCK,0);
	initOutput(SHIFT_LATCH);
	setOutDirect(SHIFT_LATCH,1);*/

	initOutput(HIGH[0]);
	setOutDirect(HIGH[0],1);
	initOutput(HIGH[1]);
	setOutDirect(HIGH[1],1);

	GPIO_InitTypeDef init;
	init.GPIO_Mode = GPIO_Mode_OUT;
	init.GPIO_OType = GPIO_OType_PP;
	init.GPIO_Pin = GPIO_Pin_All;
	init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(bB, &init);
	GPIO_Write(bB,0);

	for(int i=0;i<10;i++)
		digits[i]=symbol('_');
}

void updateDisplay()
{
	/*if(curDigit==0)
		setOutDirect(SHIFT_DATA,0);
	else
		setOutDirect(SHIFT_DATA,1);
	setOutDirect(SHIFT_CLOCK,1);
	setOutDirect(SHIFT_CLOCK,0);*/
	GPIO_Write(bB,0);
	if(curDigit<2)
	setOutDirect(HIGH[curDigit],1);
	curDigit++;
	if(curDigit>=10)
		curDigit=0;
	if(curDigit<2)
	setOutDirect(HIGH[curDigit],0);
	GPIO_Write(bB,digits[curDigit]);
	wait(5);
}
