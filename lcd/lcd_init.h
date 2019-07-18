static void SystemClock_Config(void);
void BSP_LCD_LayerInit(uint16_t LayerIndex, uint32_t FB_Address);

void LCD_Init() //Rect(0, 0, 240, 320);
{  
  BSP_LCD_Init();
  
  BSP_LCD_LayerDefaultInit(LCD_BACKGROUND_LAYER, LCD_FRAME_BUFFER+BUFFER_OFFSET);
  BSP_LCD_LayerDefaultInit(LCD_FOREGROUND_LAYER, LCD_FRAME_BUFFER);
  
  BSP_LCD_DisplayOn();

  BSP_LCD_SelectLayer(LCD_BACKGROUND_LAYER);
  BSP_LCD_SetTransparency(LCD_BACKGROUND_LAYER, 0);
  BSP_LCD_SetLayerVisible(LCD_BACKGROUND_LAYER, ENABLE);
  
  BSP_LCD_SelectLayer(LCD_FOREGROUND_LAYER);
  BSP_LCD_SetTransparency(LCD_FOREGROUND_LAYER, 0);
  BSP_LCD_SetLayerVisible(LCD_FOREGROUND_LAYER, ENABLE);
  
  SystemClock_Config();
  
  BSP_TS_Init(240, 320);
  BSP_TS_ITConfig();
}

static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  HAL_PWREx_EnableOverDrive();

  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}