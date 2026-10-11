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
#include "fsm.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TX_SIZE 1000
#define BMP180_ADDR 0xEE
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

IWDG_HandleTypeDef hiwdg;

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

volatile HAL_StatusTypeDef  bmp180_ready_status;
volatile uint32_t bmp180_i2c_error;
uint8_t bmp180_chip_id = 0xFF;
volatile HAL_StatusTypeDef bmp180_read_status;
uint8_t bmp180_cmd = 0x2E;
volatile HAL_StatusTypeDef bmp180_write_status;

uint8_t bmp180_ut_data[2] = {0xFF, 0xFF};   // 센티널 초기값
uint16_t bmp180_raw_temp = 0;
volatile HAL_StatusTypeDef bmp180_ut_status = HAL_BUSY;

uint8_t  bmp180_calib_raw[22];
volatile HAL_StatusTypeDef bmp180_calib_status = HAL_BUSY;

int16_t  AC1, AC2, AC3, B1, B2, MB, MC, MD;
uint16_t AC4, AC5, AC6;

volatile int32_t bmp180_X1, bmp180_X2, bmp180_B5;
volatile int32_t bmp180_temp_x10;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_IWDG_Init(void);
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
  MX_I2C1_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
#if 0
  HAL_TIM_Base_Start_IT(&htim2);

  bmp180_ready_status = HAL_I2C_IsDeviceReady(&hi2c1, BMP180_ADDR, 10, 1000);
  if(bmp180_ready_status!=HAL_OK){
	  bmp180_i2c_error = hi2c1.ErrorCode;
  }
  bmp180_read_status = HAL_I2C_Mem_Read(&hi2c1, BMP180_ADDR, 0xD0, I2C_MEMADD_SIZE_8BIT,
		  &bmp180_chip_id, 1, 1000);

  if(bmp180_read_status == HAL_OK && bmp180_chip_id == 0x55){
	  __NOP();
  }
  else{
	  bmp180_i2c_error = hi2c1.ErrorCode;
  }
  bmp180_write_status = HAL_I2C_Mem_Write(&hi2c1, BMP180_ADDR, 0xF4, I2C_MEMADD_SIZE_8BIT, &bmp180_cmd, 1, 1000);
  if(bmp180_write_status!=HAL_OK){
	  bmp180_i2c_error = hi2c1.ErrorCode;
  }
  HAL_Delay(5);

  bmp180_ut_status = HAL_I2C_Mem_Read(&hi2c1, BMP180_ADDR, 0xF6, I2C_MEMADD_SIZE_8BIT,
                                      bmp180_ut_data, 2, 1000);
  if (bmp180_ut_status == HAL_OK) {
      bmp180_raw_temp = ((uint16_t)bmp180_ut_data[0] << 8) | bmp180_ut_data[1];
  } else {
      bmp180_i2c_error = hi2c1.ErrorCode;
  }

  bmp180_calib_status = HAL_I2C_Mem_Read(&hi2c1, BMP180_ADDR, 0xAA, I2C_MEMADD_SIZE_8BIT,
                                         bmp180_calib_raw, 22, 1000);
  if (bmp180_calib_status == HAL_OK) {
      uint8_t *c = bmp180_calib_raw;
      AC1 = (int16_t) ((c[0]  << 8) | c[1]);
      AC2 = (int16_t) ((c[2]  << 8) | c[3]);
      AC3 = (int16_t) ((c[4]  << 8) | c[5]);
      AC4 = (uint16_t)((c[6]  << 8) | c[7]);
      AC5 = (uint16_t)((c[8]  << 8) | c[9]);
      AC6 = (uint16_t)((c[10] << 8) | c[11]);
      B1  = (int16_t) ((c[12] << 8) | c[13]);
      B2  = (int16_t) ((c[14] << 8) | c[15]);
      MB  = (int16_t) ((c[16] << 8) | c[17]);
      MC  = (int16_t) ((c[18] << 8) | c[19]);
      MD  = (int16_t) ((c[20] << 8) | c[21]);

      /* --- 온도 계산 (데이터시트 공식) --- */
      bmp180_X1 = (((int32_t)bmp180_raw_temp - (int32_t)AC6) * (int32_t)AC5) >> 15;
      bmp180_X2 = ((int32_t)MC << 11) / (bmp180_X1 + (int32_t)MD);
      bmp180_B5 = bmp180_X1 + bmp180_X2;
      bmp180_temp_x10 = (bmp180_B5 + 8) >> 4;     // 0.1°C 단위
  } else {
      bmp180_i2c_error = hi2c1.ErrorCode;
  }

  char tmsg[64];
  int tlen = snprintf(tmsg, sizeof(tmsg), "Temperature: %ld.%ld C\r\n",
                      (long)(bmp180_temp_x10 / 10), (long)(bmp180_temp_x10 % 10));
  HAL_UART_Transmit(&huart1, (uint8_t *)tmsg, tlen, 100);

  const char *teststr = "Hello STM32\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t *)teststr, strlen(teststr), 100);

  for(int i=0; i<TX_SIZE; i++){
	tx_data[i] = 'A' + i%26;
  }

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   // DWT 블록 켜기
  DWT->CYCCNT = 0;                                  // 카운터 초기화
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;              // 카운터 시작

  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
#endif
  FSM_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	FSM_Run();
#if 0
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
#endif

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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
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
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
  hiwdg.Init.Reload = 1250;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

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
  __HAL_RCC_GPIOB_CLK_ENABLE();

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

#if 0
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
#endif

volatile uint32_t tim_count = 0;   // 확인용

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        tim_count++;
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_2);
    }
}

#if 0
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
#endif

#if 0
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	if(huart->Instance == USART1){
		t_done = DWT->CYCCNT;
		tx_done = 1;
	}
}
#endif

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
