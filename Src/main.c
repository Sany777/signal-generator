/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "si5351.h"
#include "ssd1306_fonts.h"

#include "generator.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */

volatile int32_t enc_val;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */



/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  ssd1306_Init();
  si5351_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  display_help();
  HAL_Delay(1500);
  settings_t settings = {0};
  flash_read_data(&settings, sizeof(settings));

  fix_ch_data(settings.channels);
  if(settings.corection < 10000 && settings.corection > -10000){
	  si5351_set_correction(settings.corection);
  }
  HAL_Delay(1500);
  int cur_ch = 0, cursor = 0;
  int enc_data = 0, but_state;
  bool commit_changes, corection_mode = false;
  uint8_t ch_set = 0, out_val = 0;
  int accelerator = 0;
  int save_delay, reset_acelerator_delay;
  while (1)
  {

	  but_state = get_but_state();

	  enc_data = enc_val / 4;


	  commit_changes = enc_data || but_state != BUT_STATE_RELEASE;

	  if(enc_data){
		  if(cursor != CURSOR_CHANGING_POWER){
			  if(accelerator)enc_data *= accelerator;
			  accelerator += 2;
		  }

		  if(corection_mode){
			  set_correction_val(&settings, cursor, enc_data);
		  } else {
			  set_ch_val(&settings.channels[cur_ch], cursor, enc_data);
		  }

		  if(out_val){
			  if(settings.channels[cur_ch].driveStrength == SI5351_DRIVE_STRENGTH_OFF){
				  ch_set &= ~(1<<cur_ch);
			  } else {
				  si5351_SetupCLK(cur_ch, settings.channels[cur_ch].freq, settings.channels[cur_ch].driveStrength, settings.phase);
				  ch_set |= (1<<cur_ch);
			  }
		  }
	  }

	  if(but_state == BUT_STATE_LONG_PRESSED){
		  if(out_val == 0){
			  for(int i=0; i<CH_NUM; ++i){
				  if(settings.channels[i].driveStrength != SI5351_DRIVE_STRENGTH_OFF
						  	  && settings.channels[i].freq <= MAX_FREQ
							  && settings.channels[i].freq >= MIN_FREQ){
					  ch_set |= (1<<i);
					  si5351_SetupCLK(i, settings.channels[i].freq, settings.channels[i].driveStrength, settings.phase);
				  }
			  }
		  } else {
			  ch_set = 0;
		  }
	  } else if(but_state == BUT_STATE_PRESSED){
		  if(corection_mode){
			  cursor = !cursor;
		  }else{
		  	  cursor = (cursor + 1) % CURSOR_CHANGING_MAX;
		  }
	  } else if(but_state == BUT_DOUBLE_PRESSED){
		  if(corection_mode){
			  cursor = !cursor;
		  }else{
			  cur_ch = (cur_ch+1) % CH_NUM;
		  }
	  }else if(but_state == BUT_FOUR_PRESSED){
		  corection_mode = !corection_mode;
		  cursor = 0;
	  }

	  if(out_val != ch_set){
		  out_val = ch_set;
		  si5351_EnableOutputs(out_val);
	  }
	  if(corection_mode){
		  display_correction_data(settings.corection, settings.phase, cursor, ch_set);
	  }else{
	  	  display_ch_data(settings.channels, cur_ch, cursor, ch_set);
	  }

	  enc_val = 0;
	  HAL_Delay(100);
	  while(HAL_GPIO_ReadPin(gpio_key_in_GPIO_Port, gpio_key_in_Pin) == GPIO_PIN_RESET)HAL_Delay(10);
	  save_delay = 10000;
	  reset_acelerator_delay = 150;
	  while(HAL_GPIO_ReadPin(gpio_key_in_GPIO_Port, gpio_key_in_Pin) != GPIO_PIN_RESET && enc_val/4 == 0){
		  if(commit_changes){
			  if(save_delay){
				  save_delay -= 10;
			  } else {
				  commit_changes = false;
				  flash_write_data(&settings, sizeof(settings));
			  }
		  }
		  if(accelerator){
			  if(reset_acelerator_delay){
				  reset_acelerator_delay -= 10;
			  } else {
			  	  accelerator = 0;
			  }
		 }
		 HAL_Delay(10);
	  }


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : gpio_key_in_Pin */
  GPIO_InitStruct.Pin = gpio_key_in_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(gpio_key_in_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : gpio_sw1_int_Pin gpio_sw2_int_Pin */
  GPIO_InitStruct.Pin = gpio_sw1_int_Pin|gpio_sw2_int_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */

/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	static int sw1_last_pin, sw2_last_pin;
    if (GPIO_Pin == gpio_sw2_int_Pin || GPIO_Pin == gpio_sw1_int_Pin)
    {
        uint8_t sw1 = HAL_GPIO_ReadPin(GPIOA, gpio_sw1_int_Pin);
        uint8_t sw2 = HAL_GPIO_ReadPin(GPIOA, gpio_sw2_int_Pin);
        if(sw1 != sw1_last_pin || sw2 != sw2_last_pin){
			if (sw1 == sw2_last_pin) {
				enc_val += 1;
			} else {
				enc_val -= 1;
			}
		}
		sw1_last_pin = sw1;
		sw2_last_pin = sw2;
    }
}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM3 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM3) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
