/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TX_SIZE 1000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;

/* USER CODE BEGIN PV */
uint8_t rx_byte = 0;
uint8_t rx_buf[100];
uint8_t rx_index = 0;
volatile uint8_t line_ready = 0;

uint8_t tx_data[1000];

uint32_t t_start, t_end, elapsed_cycles, t_ret;
volatile uint32_t loop_count = 0;

volatile uint8_t tx_done;
volatile uint32_t t_done;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
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
  MX_DMA_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start_IT(&htim2);

  const char *teststr = "Hello STM32\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t *)teststr, strlen(teststr), 100);

  for(int i=0; i<TX_SIZE; i++){
	tx_data[i] = 'A' + i%26;
  }

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   // DWT 블록 켜기
  DWT->CYCCNT = 0;                                  // 카운터 초기화
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;              // 카운터 시작

  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	loop_count++;
	if(line_ready==1){
		HAL_UART_Transmit(&huart1, (uint8_t *)"> ", strlen("> "), 100);
		HAL_UART_Transmit(&huart1, (uint8_t *)rx_buf, rx_index, 100);
		HAL_UART_Transmit(&huart1, (uint8_t *)"\r\n", strlen("\r\n"), 100);

		if(strcmp((char *)rx_buf,"led on")==0){
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
		}
		else if(strcmp((char *)rx_buf,"led off")==0){
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
		}
		else if(strcmp((char *)rx_buf,"status")==0){
			uint8_t temp = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
			if(temp == 1){
				HAL_UART_Transmit(&huart1, (uint8_t *)"LED IS ON\r\n", strlen("LED IS ON\r\n"), 100);
			}
			else{
				HAL_UART_Transmit(&huart1, (uint8_t *)"LED IS OFF\r\n", strlen("LED IS OFF\r\n"), 100);
			}
		}
		else if(strcmp((char *)rx_buf,"send")==0){
			t_start = DWT->CYCCNT;
			uint32_t loop_count_start = loop_count;
			HAL_UART_Transmit(&huart1, tx_data, TX_SIZE, 500);
			t_end = DWT->CYCCNT;
			uint32_t loop_count_end = loop_count;
			elapsed_cycles = t_end - t_start;
			char msg[64];
			int len = sprintf(msg, "cycles = %lu, us = %lu, loopcount = %lu\r\n", (unsigned long)elapsed_cycles,
			        (unsigned long)(elapsed_cycles / 8), (unsigned long)(loop_count_end-loop_count_start));
			HAL_UART_Transmit(&huart1, (uint8_t *)msg, len, 100);
		}
		else if(strcmp((char *)rx_buf,"send it")==0){
			tx_done = 0;
			t_start = DWT->CYCCNT;
			uint32_t loop_count_start = loop_count;
			HAL_StatusTypeDef ret = HAL_UART_Transmit_IT(&huart1, tx_data, TX_SIZE);
			t_ret = DWT->CYCCNT;
			if(ret != HAL_OK){
				HAL_UART_Transmit(&huart1, (uint8_t *)"busy/error\r\n", strlen("busy/error\r\n"), 100);
			}
			else{
				while(tx_done == 0){ loop_count++; }
				uint32_t loop_count_end = loop_count;
				char msg[100];
				uint32_t ret_cycles = t_ret - t_start;
				uint32_t done_cycles = t_done - t_start;
				int len = sprintf(msg, " it ret = %lu, done = %lu, loop = %lu, us = %lu\r\n", (unsigned long)ret_cycles,
						(unsigned long)(done_cycles), (unsigned long)(loop_count_end-loop_count_start),
						(unsigned long)(done_cycles / 8));
				HAL_UART_Transmit(&huart1, (uint8_t *)msg, len, 100);
			}
		}
		else if(strcmp((char *)rx_buf,"send dma")==0){
			tx_done = 0;
			t_start = DWT->CYCCNT;
			uint32_t loop_count_start = loop_count;
			HAL_StatusTypeDef ret = HAL_UART_Transmit_DMA(&huart1, tx_data, TX_SIZE);
			t_ret = DWT->CYCCNT;
			if(ret != HAL_OK){
				HAL_UART_Transmit(&huart1, (uint8_t *)"busy/error\r\n", strlen("busy/error\r\n"), 100);
			}
			else{
				while(tx_done == 0){ loop_count++; }
				uint32_t loop_count_end = loop_count;
				char msg[100];
				uint32_t ret_cycles = t_ret - t_start;
				uint32_t done_cycles = t_done - t_start;
				int len = sprintf(msg, " DMA ret = %lu, done = %lu, loop = %lu, us = %lu\r\n", (unsigned long)ret_cycles,
						(unsigned long)(done_cycles), (unsigned long)(loop_count_end-loop_count_start),
						(unsigned long)(done_cycles / 8));
				HAL_UART_Transmit(&huart1, (uint8_t *)msg, len, 100);
			}
		}
		else if(strcmp((char *)rx_buf,"send idle")==0){
			t_start = DWT->CYCCNT;
			uint32_t loop_count_start = loop_count;

			while(DWT->CYCCNT - t_start < 690500){ loop_count++; }
			uint32_t loop_count_end = loop_count;
			char msg[100];
			int len = sprintf(msg, " idle  loop = %lu\r\n", (unsigned long)(loop_count_end-loop_count_start));
			HAL_UART_Transmit(&huart1, (uint8_t *)msg, len, 100);
		}
		else{
			HAL_UART_Transmit(&huart1, (uint8_t *)"unknown\r\n", strlen("unknown\r\n"), 100);
		}
		rx_index = 0;
		line_ready = 0;
	}

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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 79;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 9999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);

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
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_2, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA0 PA2 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
volatile uint32_t exti_count = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	static uint32_t last_tick = 0;
	if(GPIO_Pin == GPIO_PIN_1){
		uint32_t now = HAL_GetTick();
		if (now - last_tick > 50)            // 채터링 방지 (50ms 이내 재입력 무시)
		{
			last_tick = now;
			exti_count++;
			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);   // LED 상태 변경
		}
	}
}

volatile uint32_t tim_count = 0;   // 확인용

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        tim_count++;
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_2);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart->Instance == USART1){
		HAL_UART_Transmit(&huart1, &rx_byte, 1, 100);
		HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
		if(rx_byte == '\n' || rx_byte == '\r'){
			if(rx_index > 0){
				rx_buf[rx_index] = '\0';
				line_ready = 1;
			}
		}
		else{
			if(rx_index < 99){
				rx_buf[rx_index] = rx_byte;
				rx_index++;
			}
		}
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	if(huart->Instance == USART1){
		t_done = DWT->CYCCNT;
		tx_done = 1;
	}
}

/* USER CODE END 4 */

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
#ifdef USE_FULL_ASSERT
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
