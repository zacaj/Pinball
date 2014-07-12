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

int main(void)
{
  uint32_t ii;

  /* TODO - Add your application code here */
  STM_EVAL_PBInit(BUTTON_USER,BUTTON_MODE_GPIO);
  /* Example use SysTick timer and read System core clock */
  SysTick_Config(72000);  /* 1 ms if clock frequency 72 MHz */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOE, ENABLE);
 // initIO(GPIOA,makeOutInit(GPIO_Pin_0,GPIO_OType_PP,GPIO_Speed_Level_3));
  //initIO(GPIOF,makeOutInit(GPIO_Pin_0,GPIO_OType_PP,GPIO_Speed_Level_3));
  initIO(GPIOD,makeOutInit(GPIO_Pin_0,GPIO_OType_PP,GPIO_Speed_Level_3));
  initIO(GPIOE,makeOutInit(GPIO_Pin_7,GPIO_OType_PP,GPIO_Speed_Level_3));
  //initIO(GPIOA,makeInInit(GPIO_Pin_1,GPIO_PuPd_DOWN));

  {
	  	NVIC_InitTypeDef NVIC_InitStructure;
		/* Enable the TIM2 gloabal Interrupt */
		NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);

		/* TIM2 clock enable */
		TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);
		/* Time base configuration */
		TIM_TimeBaseStructure.TIM_Period = 2*(80*(1)) - 1;  // 1 MHz down to 1 KHz (1 ms)
		TIM_TimeBaseStructure.TIM_Prescaler = 36000 - 1; // 24 MHz Clock down to 1 MHz (adjust per your clock)
		TIM_TimeBaseStructure.TIM_ClockDivision = 0;
		TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
		TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure);
		/* TIM IT enable */
		TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE);
		//1,8,2,7,3,4,6,15,16,17
		/* TIM2 enable counter */
  }


  SystemCoreClockUpdate();
  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);
  ii = SystemCoreClock;   /* This is a way to read the System core clock */
  /* Example update ii when timerFlag has been set by SysTick interrupt */
  ii = 0;
  int i=6;
  int on=0;
  while (1)
  {
    if (msTicks>600)
    {
    	msTicks=0;
     // timerFlag = 0;
      //ii++;
      //STM_EVAL_LEDToggle(LED3);
      //on=!on;
     // GPIO_WriteBit(pins[i].GPIOx,pins[i].pin,on);
    	STM_EVAL_LEDOn(LED3);
    				GPIO_WriteBit(GPIOE,GPIO_Pin_7,1);
    				GPIO_WriteBit(GPIOD,GPIO_Pin_0,1);
    				TIM_SetCounter(TIM7,0);
    				TIM_Cmd(TIM7, ENABLE);
    			    TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
    }
   /* if(GPIO_ReadInputDataBit(pins[i].GPIOx,pins[i].pin))
    	STM_EVAL_LEDOn(LED4);
    else
    	STM_EVAL_LEDOff(LED4);*/
    if(buttonState!=STM_EVAL_PBGetState(BUTTON_USER))
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
			STM_EVAL_LEDToggle(LED3);
			GPIO_WriteBit(GPIOE,GPIO_Pin_7,1);
			GPIO_WriteBit(GPIOD,GPIO_Pin_0,1);
			TIM_SetCounter(TIM7,0);
			TIM_Cmd(TIM7, ENABLE);
		    TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
    	}
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
