#include "main.h"
#include "stm32f429xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ts.h"
#include "stdio.h"
#include "FFT.h"
#include "lcd/lcd_init.h"
#include "stm32f4xx_hal_adc.h"

ADC_HandleTypeDef hadc;
ADC_ChannelConfTypeDef adc_ch;

void DrawPage();
void SetTimers();
void GpioInit();
void InitColors();
void ADC_Init();
void analysis();

#define N 4096
#define PART 1024

float w = 0.4;

unsigned int BUFF[N];

float buf[N];
float Im[N];
float amplf[250];
float block[250];

float middle = 0;

unsigned int freq[250];
unsigned int Color[18];

unsigned int fps = 0;
unsigned int frames = 0;

unsigned int time = 0;

unsigned int tps = 0;
unsigned int ticks = 0;

unsigned int index = 0;

unsigned int current_block = 0;
unsigned int max_freq = 0;

unsigned int switcher = 2;

unsigned char stop = 0;
unsigned char ready = 1;
unsigned char pointer = 1;
unsigned int last_push = 0;

float mod(float value)
{
  if(value < 0)
  {
    return -value;
  }
  
  return value;
}

void main()
{
  HAL_Init();
  
  InitColors();
  GpioInit();
  LCD_Init();
  ADC_Init();
  SetTimers();
  
  while(1)
  {
    
  }
}

void InitColors()
{
  Color[0] =    0xFF4F4F4F;
  Color[1] =    0xFF8295EB;
  Color[2] =    0xFF669CE6;
  Color[3] =    0xFF63EDBE;
  Color[4] =    0xFF6BEF5D;
  Color[5] =    0xFF36DE24;
  Color[6] =    0xFF5BE301;
  Color[7] =    0xFFffC32A;
  Color[8] =    0xFFF49000;
  Color[9] =    0xFFFF903F;
  Color[10] =   0xFFFF563F;
  Color[11] =   0xFFFF1924;
  Color[12] =   0xFFFF1356;
  Color[13] =   0xFFEF00F2;
  Color[14] =   0xFF707085;
}

void GpioInit()
{
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOGEN;
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
  
  GPIOA->MODER |= GPIO_MODER_MODER7;
  
  GPIOG->MODER |= GPIO_MODER_MODER13_0;
  GPIOG->MODER &=~ GPIO_MODER_MODER13_1;
  
  GPIOG->MODER |= GPIO_MODER_MODER14_0;
  GPIOG->MODER &=~ GPIO_MODER_MODER14_1;
  
  GPIOA->MODER &=~ GPIO_MODER_MODER0_0;
  GPIOA->MODER &=~ GPIO_MODER_MODER0_1;
  
  EXTI->IMR  |= EXTI_IMR_MR0;
  EXTI->RTSR |= EXTI_RTSR_TR0;
  
  NVIC_EnableIRQ(EXTI0_IRQn);
}

void EXTI0_IRQHandler()
{  
  EXTI->PR |= EXTI_PR_PR0;
  
  if(stop == 0)
  {
    stop = 1;
    NVIC_DisableIRQ(TIM3_IRQn);
    
    for(int i=0; i<N; ++i)
    {
      buf[i] = 0;
      Im[i] = 0;
      index = 0;
    }
  }
  else
  {
    stop = 0;
    NVIC_EnableIRQ(TIM3_IRQn);
  }
}

void SetTimers()
{
  //Милисекундный таймер:
  
  RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
  
  TIM4->ARR   = 83;
  TIM4->PSC   = 999;
  
  TIM4->DIER |= TIM_DIER_UIE;
  TIM4->CR1  |= TIM_CR1_ARPE;
  TIM4->CR1  |= TIM_CR1_CEN;
  
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
  
  int ARR = (int)(84000000.0*w/N+0.5)-1;
  int PSC = 0;
  
  for(int i=0; ARR > 65535; i++)
  {
    ARR = (int)((ARR+1)/2.0+0.5)-1;
    PSC = PSC*2+1;
  }
  
  w = (ARR+1)*N*(PSC+1)/84000000.0;
  
  TIM3->ARR = ARR;
  TIM3->PSC = PSC;
  
  TIM3->DIER |= TIM_DIER_UIE;
  TIM3->CR1  |= TIM_CR1_ARPE;
  TIM3->CR1 |= TIM_CR1_CEN;
  
  NVIC_EnableIRQ(TIM3_IRQn);
  NVIC_EnableIRQ(TIM4_IRQn);
}

void TIM4_IRQHandler()
{ 
  TIM4->SR &=~ TIM_SR_UIF;
  
  time++;
  
  if(time && time % 40 == 0)
  {
     DrawPage();
  }
  
  if(time && time % 1000 == 0)
  {
    
    if(frames > 99)
    {
      frames = 99;
    }
    
    tps = ticks;
    ticks = 0;
    
    fps = frames;
    frames = 0;
  }
  
  if(time == 100000)
  {
    time = 0;
  }
}

void EXTI15_10_IRQHandler(void)
{  
	EXTI->PR |= EXTI_IMR_MR15;

	BSP_TS_ITClear();

	TS_StateTypeDef TSState;
	BSP_TS_GetState(&TSState);

	int x = TSState.X;
	int y = TSState.Y;

	if(x>=63 && x<=103 && y>=10 && y<=40)
	{
		if(pointer)
		{
			pointer = 0;
		}
		else
		{
			pointer = 1;
		}

		return;
	}

	if(x>=111 && x<=141 && y>=10 && y<=40)
	{
		switcher = 1;
		index = 0;
		freq[current_block] = 0;

		for(int i=0; i<240; ++i)
		{
			block[i] = 0;
		}

		return;
	}

	if(x>=160 && x<=200 && y>=10 && y<=40)
	{
		switcher = 2;
		index = 0;
		freq[current_block] = 0;

		for(int i=0; i<240; ++i)
		{
			block[i] = 0;
		}

		return;
	}

	if(pointer && stop)
	{
		if(mod(time - last_push) > 100-(switcher-1)*70)
		{
			last_push = time;

			if(switcher == 1)
			{

				if(y >= 5 && y <= 160 && x >= 5 && x <= 230)
				{
					if(current_block < 19)
					{
						current_block++;
						return;
					}
				}

				if(y >= 161 && y <= 310 && x >= 5 && x <= 230)
				{
					if(current_block > 0)
					{
						current_block--;
						return;
					}
				}

			}

			if(switcher == 2)
			{

				if(y >= 5 && y <= 160 && x >= 5 && x <= 230)
				{
					if(current_block < 261)
					{
						current_block++;
						return;
					}
				}

				if(y >= 161 && y <= 310 && x >= 5 && x <= 230)
				{
					if(current_block > 0)
					{
						current_block--;
						return;
					}
				}

			}

		}
	}
}

void DrawPage() //0, 0, 240, 320;
{
	unsigned char sim[5] = {"\0"};

	_itoa(sim, fps, 1);

	static char flag = 1;  

	BSP_LCD_SelectLayer(flag);

	BSP_LCD_SetTransparency(flag, 0);

	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);

	BSP_LCD_Clear(LCD_COLOR_WHITE);

	BSP_LCD_SetFont(&Font24);

	if(switcher <= 2)
	{
		BSP_LCD_DrawRect(5, 10, 50, 30);

		BSP_LCD_DisplayStringAt(15, 15, sim, LEFT_MODE);

		BSP_LCD_DrawRect(63, 10, 40, 30);

		if(pointer == 1)
		{
			BSP_LCD_SetTextColor(LCD_COLOR_GRAY);
			BSP_LCD_SetBackColor(LCD_COLOR_GRAY);
			BSP_LCD_FillRect(64, 11, 39, 29);
			BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
			BSP_LCD_DisplayStringAt(78, 15, "A", LEFT_MODE);
			BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
			BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
		}
		else
		{
			BSP_LCD_DisplayStringAt(78, 15, "A", LEFT_MODE);
		}  

		BSP_LCD_DrawRect(111, 10, 40, 30);

		if(switcher == 1)
		{
			BSP_LCD_SetTextColor(LCD_COLOR_GRAY);
			BSP_LCD_SetBackColor(LCD_COLOR_GRAY);
			BSP_LCD_FillRect(112, 11, 39, 29);
			BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
			BSP_LCD_DisplayStringAt(123, 15, "1", LEFT_MODE);
			BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
			BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
		}
		else
		{
			BSP_LCD_DisplayStringAt(123, 15, "1", LEFT_MODE);
		}

		BSP_LCD_DrawRect(159, 10, 41, 30);

		if(switcher == 2)
		{
			BSP_LCD_SetTextColor(LCD_COLOR_GRAY);
			BSP_LCD_SetBackColor(LCD_COLOR_GRAY);
			BSP_LCD_FillRect(160, 11, 40, 29);
			BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
			BSP_LCD_DisplayStringAt(172, 15, "2", LEFT_MODE);
			BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
			BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
		}
		else
		{
			BSP_LCD_DisplayStringAt(172, 15, "2", LEFT_MODE);
		}

		if(switcher == 1)
		{
			BSP_LCD_DrawRect(5, 50, 195, 262);

			if(pointer == 1)
			{
				if(freq[current_block])
				{
					BSP_LCD_SetTextColor(Color[14]);

					BSP_LCD_FillRect(19, 300-current_block*13-1, 181, 12); 
				}
			}

			for(char a=0; a<20; ++a)
			{ 
				for(char h=0; h<(int)(block[a]+0.5); ++h)
				{
					BSP_LCD_SetTextColor(Color[h]);

					BSP_LCD_FillRect(190-h*13, 300-a*13, 10, 10);
				}
			}
		}

		if(switcher == 2)
		{
			BSP_LCD_DrawRect(5, 50, 195, 262);

			BSP_LCD_SetTextColor(LCD_COLOR_RED);

			for(int a=0; a<240; ++a)
			{
				BSP_LCD_DrawLine(199, 311-a-10, 199-(int)(block[a]+0.5), 311-a-10);
			}

			BSP_LCD_SetTextColor(Color[0]);
			BSP_LCD_DrawLine(199-(int)(block[current_block]+0.5), 301-current_block, 199, 301-current_block);
		}
	}

	if(switcher && switcher <= 2)
	{
		BSP_LCD_SetFont(&Font12);
		BSP_LCD_SetTextColor(LCD_COLOR_BLACK);

		if(pointer == 0)
		{

			for(int i=0; i<20; ++i)
			{
				for(int c=0; c<8; ++c)
				{
					sim[c] = 0;
				}
				_itoa(sim, (i+1)*250, 1);

				BSP_LCD_DisplayStringAt(208, 300-i*13, sim, LEFT_MODE);
			}
		}
		else
		{

			for(int c=0; c<8; ++c)
			{
				sim[c] = 0;
			}

			if(switcher == 1)
			{

				if(current_block == 0)
				{
					_itoa(sim, freq[0], 1);
				}
				else
				{
					_itoa(sim, 250, 1);
				}   

				BSP_LCD_DisplayStringAt(208, 300, sim, LEFT_MODE);

				for(int c=0; c<8; ++c)
				{
					sim[c] = 0;
				}

				if(current_block == 19)
				{
					_itoa(sim, freq[19], 1);
				}
				else
				{
					_itoa(sim, 5000, 1);
				}

				BSP_LCD_DisplayStringAt(208, 53, sim, LEFT_MODE);

				if(current_block && current_block < 19)
				{
					for(int c=0; c<8; ++c)
					{
						sim[c] = 0;
					}

					_itoa(sim, freq[current_block], 1);

					BSP_LCD_DisplayStringAt(208, 300-current_block*13, sim, LEFT_MODE);
				}
			}

			if(switcher == 2)
			{
				if(current_block == 0)
				{
					_itoa(sim, freq[0], 1);
				}
				else
				{
					_itoa(sim, 200, 1);
				}

				BSP_LCD_DisplayStringAt(208, 300, sim, LEFT_MODE);    

				for(int c=0; c<8; ++c)
				{
					sim[c] = 0;
				}

				if(current_block == 239)
				{
					_itoa(sim, freq[239], 1);
				}
				else
				{
					_itoa(sim, 5000, 1);
				}

				BSP_LCD_DisplayStringAt(208, 53, sim, LEFT_MODE);

				if(current_block && current_block < 239)
				{
					for(int c=0; c<8; ++c)
					{
						sim[c] = 0;
					}

					_itoa(sim, freq[current_block], 1);

					char u = 15;

					if(current_block > 15)
					{
						u = current_block;
					}

					BSP_LCD_DisplayStringAt(208, 301-u, sim, LEFT_MODE);
				}
			}

			unsigned char str[10] = {"\0"};

			int A = 0;

			if(switcher == 1)
			{
				A = (int)(amplf[current_block]+0.5);
			}
			else
			{
				A = (int)(amplf[current_block]*100.0+0.5);
			}

			_itoa(str, A, 1);

			BSP_LCD_DisplayStringAt(208, 300-20*13, str, LEFT_MODE);
		}
	}

	BSP_LCD_SetTransparency(flag, 255);

	BSP_LCD_SetTransparency(!flag, 0);

	flag = !flag;

	frames++;
}

void ADC_Init()
{
	__HAL_RCC_ADC1_CLK_ENABLE();
	HAL_NVIC_EnableIRQ(ADC_IRQn);

	GPIOA->MODER |= GPIO_MODER_MODER7_1;
	GPIOA->MODER |= GPIO_MODER_MODER7_0;

	hadc.Instance = ADC1;
	hadc.Init.Resolution = ADC_RESOLUTION_12B;
	hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc.Init.ScanConvMode = ENABLE;
	hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc.Init.ContinuousConvMode = DISABLE;
	hadc.Init.NbrOfConversion = 1;

	HAL_ADC_Init(&hadc);

	adc_ch.Channel = ADC_CHANNEL_7;
	adc_ch.Rank = 1;

	HAL_ADC_ConfigChannel(&hadc, &adc_ch);
	HAL_ADC_Start(&hadc);

	for(int i=0; i<1024; ++i)
	{
		HAL_ADC_Start(&hadc);
		HAL_ADC_GetValue(&hadc);
	}

}

void TIM3_IRQHandler()
{ 
	TIM3->SR &=~ TIM_SR_UIF;

	if(index == N)
	{
		if(ready)
		{
			ready = 0;

			analysis();

			for(int i=0; i<N-PART; ++i)
			{
				BUFF[i] = BUFF[i+PART];
			}

			for(int i=N-PART; i<N; ++i)
			{
				BUFF[i] = 0;
			}

			index = N-PART;

			ready = 1;

			ticks++;
		}
	}

	else
	{
		HAL_ADC_Start(&hadc);
		BUFF[index] = HAL_ADC_GetValue(&hadc);
		index++;
	}
}

void analysis()
{  
	double sum = 0;

	float k = 2.5;

	for(int i=0; i<N; ++i)
	{
		buf[i] = BUFF[i];
		sum += buf[i];
		Im[i] = 0;
	}

	sum = sum/N;

	for(int i=0; i<N; ++i)
	{
		buf[i] = buf[i] - sum;
	}

	FFT(buf, Im, N, FT_DIRECT);

	for(int i=0; i<N/2; ++i)
	{
		Im[i] = _sqrt(Im[i]*Im[i]+buf[i]*buf[i]);
	}

	if(switcher == 1)
	{
		unsigned int j = 0;

		float sum = 0;  
		float max = 0;

		float max_freq_amplf = 0;

		for(int i=40; i<100; ++i)
		{
			float th = Im[i];

			amplf[0] += Im[i];

			if(th>max)
			{
				max = th;
				j = i;
			}

			if(th > max_freq_amplf)
			{
				max_freq_amplf = th;
				max_freq = (int)(i*k+0.5);
			}
		}

		amplf[0] /= 60.0;

		freq[0] = (int)(j*k+0.5);

		for(int i=1; i<20; ++i)
		{
			max = 0;

			for(int c=i*100; c<(i+1)*100; ++c)
			{
				float th = Im[c];

				amplf[i] += th;

				if(th>max)
				{
					max = th;
					j = c;
				}

				if(th > max_freq_amplf)
				{
					max_freq_amplf = th;
					max_freq = (int)(c*k+0.5);
				}
			}

			amplf[i] /= 100.0;
			freq[i] = (int)(j*k+0.5);
		}

		max = 0;
		sum = 0;

		for(int i=0; i<20; ++i)
		{
			if(amplf[i] > max)
			{
				max = amplf[i];
			}
		}

		for(int i=0; i<20; ++i)
		{
			block[i] = (amplf[i]/max*14.0);
			sum = sum + block[i];
		}

		for(int i=0; i<20; ++i)
		{
			amplf[i] = (int)((float)block[i]/sum*100+0.5);

			if(amplf[i] > 99)
			{
				amplf[i] = 99;
			}
		}

		current_block = (int)(max_freq/250);
	}

	else
	{
		unsigned int j = 0;

		float sum = 0;
		float max = 0;

		float max_freq_amplf = 0;

		for(int i=0; i<240; ++i)
		{
			max = 0;

			for(int c=i*8+80; c<(i+1)*8+80; ++c)
			{
				float th = Im[c];

				amplf[i] += th;

				if(th>max)
				{
					max = th;
					j = c;
				}
			}

			amplf[i] /= 8.0;

			if(amplf[i] > max_freq_amplf)
			{
				max_freq_amplf = amplf[i];
				max_freq = (int)((i*8+4)*k+0.5);
			}

			freq[i] = (int)(j*k+0.5);
		}

		max = 0;
		sum = 0;

		for(int i=0; i<240; ++i)
		{
			if(amplf[i] > max)
			{
				max = amplf[i];
				current_block = i;
			}
		}

		for(int i=0; i<240; ++i)
		{
			block[i] = (amplf[i]/max*180.0);
			sum = sum + block[i];
		}

		for(int i=0; i<240; ++i)
		{
			amplf[i] = block[i]/sum*100;
		}
	}
}

