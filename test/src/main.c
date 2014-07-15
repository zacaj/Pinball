/**
*****************************************************************************
**
**  File        : main.c
**
**  Abstract    : main function.
**
**  Functions   : main
**
**  Environment : Atollic TrueSTUDIO(R)
**
**  Distribution: The file is distributed �as is,� without any warranty
**                of any kind.
**
**  (c)Copyright Atollic AB.
**  You may use this file as-is or modify it according to the needs of your
**  project. This file may only be built (assembled or compiled and linked)
**  using the Atollic TrueSTUDIO(R) product. The use of this file together
**  with other tools than Atollic TrueSTUDIO(R) is not permitted.
**
*****************************************************************************
*/

/* Includes */
#include <stddef.h>
#include "stm32f30x.h"
#include "stm32f30x_gpio.h"
#include "stm32f30x_conf.h"
#include "stm32f3_discovery.h"
#include <stdio.h>
#include "timer.h"
/* Private typedef */

/* Private define  */

/* Private macro */

/* Private variables */
uint32_t buttonState=0;

/* Private function prototypes */

GPIO_InitTypeDef makeIOInit(uint32_t pin, GPIOMode_TypeDef mode,GPIOPuPd_TypeDef def,GPIOOType_TypeDef type, GPIOSpeed_TypeDef speed);
GPIO_InitTypeDef makeOutInit(uint32_t pin,GPIOOType_TypeDef type, GPIOSpeed_TypeDef speed);
GPIO_InitTypeDef makeInInit(uint32_t pin,GPIOPuPd_TypeDef def);
void initIO(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef init);
/* Private functions */

/* Global variables */
uint32_t timerFlag=0;


/*
 * no out: PF0,1
 * half out: PE8-15 (led)
 *
 * check:
 * PE0-5 PC14,15 PB3,6,7 PA5-7,11-14
 */
typedef struct
{
	GPIO_TypeDef* GPIOx;
	uint32_t pin;
} IOPin;

IOPin pins[8]={
#define A GPIOA
#define B GPIOB
#define C GPIOC
#define D GPIOD
#define E GPIOE
#define F GPIOF
#define P0 GPIO_Pin_0
#define P1 GPIO_Pin_1
#define P2 GPIO_Pin_2
#define P3 GPIO_Pin_3
#define P4 GPIO_Pin_4
#define P5 GPIO_Pin_5
#define P6 GPIO_Pin_6
#define P7 GPIO_Pin_7
#define P8 GPIO_Pin_8
#define P9 GPIO_Pin_9
#define P10 GPIO_Pin_10
#define P11 GPIO_Pin_11
#define P12 GPIO_Pin_12
#define P13 GPIO_Pin_13
#define P14 GPIO_Pin_14
#define P15 GPIO_Pin_15
		{C,P7},
		{D,P14},
		{D,P12},
		{D,P10},
		{D,P8},
		{B,P14},
		{B,P12},
		{B,P10},
		/*{GPIOA,GPIO_Pin_5},//out	in
		{GPIOA,GPIO_Pin_6},//out	in
		{GPIOA,GPIO_Pin_7},//out	in
		{GPIOA,GPIO_Pin_11},//0/0
		{GPIOA,GPIO_Pin_12},//0/0
		{GPIOA,GPIO_Pin_13},//1.5/3  3/3 break?
		{GPIOA,GPIO_Pin_14},//0/0	in  break?

		{GPIOB,GPIO_Pin_3},//0/0
		{GPIOB,GPIO_Pin_6},//3/3
		{GPIOB,GPIO_Pin_7},//3/3	3/3

		{GPIOC,GPIO_Pin_14},//0/0
		{GPIOC,GPIO_Pin_15},//0/0

		{GPIOE,GPIO_Pin_0},//0/.4	in
		{GPIOE,GPIO_Pin_1},//0/.4	in
		{GPIOE,GPIO_Pin_2},//2.4/3	3/3
		{GPIOE,GPIO_Pin_3},//out	in
		{GPIOE,GPIO_Pin_4},//0/.4	in
		{GPIOE,GPIO_Pin_5},//0/0	in*/
#undef A
#undef B
#undef C
#undef D
#undef E
#undef F
#undef P0
#undef P1
#undef P2
#undef P3
#undef P4
#undef P5
#undef P6
#undef P7
#undef P8
#undef P9
#undef P10
#undef P11
#undef P12
#undef P13
#undef P14
#undef P15
};
/**
 * 62 io
 * 4 i
 * 8 led
 * 1 button
 * 10 bad
 *
**===========================================================================
**
**  Abstract: main program
**
**===========================================================================
*/
uint32_t msTicks = 0,msElapsed=0;                                       /* Variable to store millisecond ticks */

uint32_t ii;
//1,8,2,7,3,4,6,15,16,17
//return 1 to disable timer, 0 to repeat

uint32_t disableLight()
{
	GPIO_WriteBit(pins[ii].GPIOx,pins[ii].pin,0);
	STM_EVAL_LEDToggle(LED3+ii);
	return 1;
}

int main(void)
{

  /* TODO - Add your application code here */
  STM_EVAL_PBInit(BUTTON_USER,BUTTON_MODE_GPIO);
  /* Example use SysTick timer and read System core clock */
  SysTick_Config(72000);  /* 1 ms if clock frequency 72 MHz */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOE, ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOF, ENABLE);
 // initIO(GPIOA,makeOutInit(GPIO_Pin_0,GPIO_OType_PP,GPIO_Speed_Level_3));
  //initIO(GPIOF,makeOutInit(GPIO_Pin_0,GPIO_OType_PP,GPIO_Speed_Level_3));
  //initIO(GPIOD,makeOutInit(GPIO_Pin_0,GPIO_OType_PP,GPIO_Speed_Level_3));
  //initIO(GPIOE,makeOutInit(GPIO_Pin_7,GPIO_OType_PP,GPIO_Speed_Level_3));
  int i;
  for(i=0;i<8;i++)
	  initIO(pins[i].GPIOx,makeOutInit(pins[i].pin,GPIO_OType_PP,GPIO_Speed_Level_3));
  //initIO(GPIOA,makeInInit(GPIO_Pin_1,GPIO_PuPd_DOWN));
  initTimers();
  timerFuncs[5]=disableLight;


  setTimer(5,90);

  SystemCoreClockUpdate();
  for(i=0;i<8;i++)
	  STM_EVAL_LEDInit(LED3+i);
  //ii = SystemCoreClock;   /* This is a way to read the System core clock */
  /* Example update ii when timerFlag has been set by SysTick interrupt */
  ii = 0;
  int on=0;
  while (1)
  {
    if (msTicks>500)
    {
    	msTicks=0;
    	ii++;
    	if(ii>=8)
    		ii=0;
    	STM_EVAL_LEDOn(LED3+ii);
		GPIO_WriteBit(pins[ii].GPIOx,pins[ii].pin,1);
		startTimer(5);
    }
   /* if(GPIO_ReadInputDataBit(pins[i].GPIOx,pins[i].pin))
    	STM_EVAL_LEDOn(LED4);
    else
    	STM_EVAL_LEDOff(LED4);*/
   /* if(buttonState!=STM_EVAL_PBGetState(BUTTON_USER))
    {
    	buttonState=STM_EVAL_PBGetState(BUTTON_USER);
    	if(buttonState)
    	{
			i++;
			/*if(i==18)
				i=0;
			//STM_EVAL_LEDToggle(LED3);
			printf("%i\n",i);
			initIO(pins[i].GPIOx,makeOutInit(pins[i].pin,GPIO_OType_PP,GPIO_Speed_Level_3));
			//GPIO_WriteBit(GPIOF,GPIO_Pin_0,buttonState);
			//GPIO_WriteBit(GPIOF,GPIO_Pin_1,buttonState);*/
			/*STM_EVAL_LEDToggle(LED3);
			GPIO_WriteBit(GPIOE,GPIO_Pin_7,1);
			GPIO_WriteBit(GPIOD,GPIO_Pin_0,1);
			TIM_SetCounter(TIM7,0);
			TIM_Cmd(TIM7, ENABLE);
		    TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
    	}
    }*/
  }
  /*
   * void STM_EVAL_PBInit(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode);
uint32_t STM_EVAL_PBGetState(Button_TypeDef Button);
   */
  /* Program will never run to this line */
  return 0;
}

GPIO_InitTypeDef makeIOInit(uint32_t pin, GPIOMode_TypeDef mode,GPIOPuPd_TypeDef def,GPIOOType_TypeDef type, GPIOSpeed_TypeDef speed)
{
	GPIO_InitTypeDef info;
	info.GPIO_Mode=mode;
	info.GPIO_OType=type;
	info.GPIO_Pin=pin;
	info.GPIO_PuPd=def;
	info.GPIO_Speed=speed;
	return info;
}
GPIO_InitTypeDef makeOutInit(uint32_t pin,GPIOOType_TypeDef type, GPIOSpeed_TypeDef speed)
{
	GPIO_InitTypeDef info;
	info.GPIO_Mode=GPIO_Mode_OUT;
	info.GPIO_OType=type;
	info.GPIO_Pin=pin;
	info.GPIO_PuPd=GPIO_PuPd_NOPULL;
	info.GPIO_Speed=speed;
	return info;
}
GPIO_InitTypeDef makeInInit(uint32_t pin,GPIOPuPd_TypeDef def)
{
	GPIO_InitTypeDef info;
	info.GPIO_Mode=GPIO_Mode_IN;
	info.GPIO_OType=GPIO_OType_PP;
	info.GPIO_Pin=pin;
	info.GPIO_PuPd=def;
	info.GPIO_Speed=GPIO_Speed_2MHz;
	return info;
}

void initIO(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef init)
{
	GPIO_Init(GPIOx,&init);
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/*
 * Minimal __assert_func used by the assert() macro
 * */
void __assert_func(const char *file, int line, const char *func, const char *failedexpr)
{
  while(1)
  {}
}

/*
 * Minimal __assert() uses __assert__func()
 * */
void __assert(const char *file, int line, const char *failedexpr)
{
   __assert_func (file, line, NULL, failedexpr);
}

#ifdef USE_SEE
#ifndef USE_DEFAULT_TIMEOUT_CALLBACK
/**
  * @brief  Basic management of the timeout situation.
  * @param  None.
  * @retval sEE_FAIL.
  */
uint32_t sEE_TIMEOUT_UserCallback(void)
{
  /* Return with error code */
  return sEE_FAIL;
}
#endif
#endif /* USE_SEE */


/**
  * @brief  Basic management of the timeout situation.
  * @param  None.
  * @retval sEE_FAIL.
  */
void EVAL_AUDIO_TransferComplete_CallBack(uint32_t pBuffer, uint32_t Size)

{
  /* TODO, implement your code here */
  return;
}
