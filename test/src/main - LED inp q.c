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

IOPin pins[18]={
		{GPIOA,GPIO_Pin_5},//out	in
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
		{GPIOE,GPIO_Pin_5},//0/0	in
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

void TIM7_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET)
	{
		GPIO_WriteBit(GPIOE,GPIO_Pin_7,0);
		GPIO_WriteBit(GPIOD,GPIO_Pin_0,0);
		TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
		STM_EVAL_LEDToggle(LED3);
		TIM_Cmd(TIM7, DISABLE);
	}
}
void wait(uint32_t ms)
{
	uint32_t start=msElapsed;
	while(msElapsed<start+ms);
}
int main(void)
{
  uint32_t ii;

  /* TODO - Add your application code here */
  STM_EVAL_PBInit(BUTTON_USER,BUTTON_MODE_GPIO);
  /* Example use SysTick timer and read System core clock */
  SysTick_Config(72000);  /* 1 ms if clock frequency 72 MHz */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOE, ENABLE);
 // initIO(GPIOA,makeOutInit(GPIO_Pin_0,GPIO_OType_PP,GPIO_Speed_Level_3));
  //initIO(GPIOF,makeOutInit(GPIO_Pin_0,GPIO_OType_PP,GPIO_Speed_Level_3));
  //initIO(GPIOD,makeOutInit(GPIO_Pin_0,GPIO_OType_PP,GPIO_Speed_Level_3));
#define LED_CLOCK GPIO_Pin_3
#define LED_DATA GPIO_Pin_6
#define LED_LATCH GPIO_Pin_7
  initIO(GPIOD,makeOutInit(LED_CLOCK,GPIO_OType_PP,GPIO_Speed_Level_3));
  GPIO_WriteBit(GPIOD,LED_CLOCK,0);
  initIO(GPIOD,makeOutInit(LED_DATA,GPIO_OType_PP,GPIO_Speed_Level_3));
  GPIO_WriteBit(GPIOD,LED_DATA,0);
  initIO(GPIOD,makeOutInit(LED_LATCH,GPIO_OType_PP,GPIO_Speed_Level_3));
  GPIO_WriteBit(GPIOD,LED_LATCH,0);

#define IN_CLOCK GPIO_Pin_7
#define IN_DATA GPIO_Pin_0
#define IN_LATCH GPIO_Pin_8
  initIO(GPIOE,makeOutInit(IN_CLOCK,GPIO_OType_PP,GPIO_Speed_Level_3));
  GPIO_WriteBit(GPIOE,IN_CLOCK,0);
  initIO(GPIOB,makeInInit(IN_DATA,GPIO_PuPd_DOWN));
  initIO(GPIOB,makeOutInit(IN_LATCH,GPIO_OType_PP,GPIO_Speed_Level_3));
  GPIO_WriteBit(GPIOB,IN_LATCH,1);
  //initIO(GPIOA,makeInInit(GPIO_Pin_1,GPIO_PuPd_DOWN));

  SystemCoreClockUpdate();
  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);
  ii = SystemCoreClock;   /* This is a way to read the System core clock */
  /* Example update ii when timerFlag has been set by SysTick interrupt */
  ii = 0x88;
  int on=0;
  while (1)
  {
    if (msTicks>200)
    {
    	msTicks=0;

    	/*{
    		uint32_t in=0;
    		GPIO_WriteBit(GPIOB,IN_LATCH,0);
    		int i;
    		for(i=0;i<8;i++)
    		{
	    		GPIO_WriteBit(GPIOE,IN_CLOCK,0);
				in<<=1;
				in|=GPIO_ReadInputDataBit(GPIOB,IN_DATA);
	    		GPIO_WriteBit(GPIOE,IN_CLOCK,1);
    		}
    		GPIO_WriteBit(GPIOB,IN_LATCH,1);
    		on=in>>8;
    		ii=in;
    	}*/

    	//on=!on;
    	if(on)
			STM_EVAL_LEDOn(LED4);
		else
			STM_EVAL_LEDOff(LED4);
    	int i;
    	ii<<=1;
    	ii|=(ii&16)>>4;
    	uint32_t start=msElapsed;
    	for(i=0;i<8;i++)
    	{
    		GPIO_WriteBit(GPIOD,LED_DATA,ii&1<<i);//on);//
    		GPIO_WriteBit(GPIOD,LED_CLOCK,1);
    		//wait(1);
    		GPIO_WriteBit(GPIOD,LED_CLOCK,0);
    	}
    	for(i=0;i<8;i++)
    	{
    		GPIO_WriteBit(GPIOD,LED_DATA,(ii>>8)&1<<i);//on);//
    		GPIO_WriteBit(GPIOD,LED_CLOCK,1);
    		//wait(1);
    		GPIO_WriteBit(GPIOD,LED_CLOCK,0);
    	}
    	wait((msElapsed-start)*2);
		GPIO_WriteBit(GPIOD,LED_DATA,0);
		GPIO_WriteBit(GPIOD,LED_LATCH,1);
		//wait(1);
		GPIO_WriteBit(GPIOD,LED_LATCH,0);
    }
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
	info.GPIO_PuPd=GPIO_PuPd_DOWN;
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
