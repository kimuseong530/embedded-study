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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void DEC_Select(uint8_t n);
void DEC_Enable(uint8_t on);
void DEC_On(uint8_t n);
void DEC_Off(void);
void Demo_Count(uint32_t ms);
void Demo_Knight(uint32_t ms);
void Demo_Pattern(uint8_t pattern, uint32_t hold_ms);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* 선택 코드만 바꾼다 (출력 허가는 건드리지 않음). n = 0 ~ 7 */
void DEC_Select(uint8_t n)
{
  HAL_GPIO_WritePin(A_1_GPIO_Port, A_1_Pin, (n & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(B_2_GPIO_Port, B_2_Pin, (n & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(C_3_GPIO_Port, C_3_Pin, (n & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* 출력 허가 / 금지 (G1) */
void DEC_Enable(uint8_t on)
{
  HAL_GPIO_WritePin(G1_GPIO_Port, G1_Pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* LEDn 하나만 켜기. 바꾸기 전에 G1을 내렸다가 다시 올려 고스팅을 없앤다. */
void DEC_On(uint8_t n)
{
  DEC_Enable(0);
  DEC_Select(n);
  DEC_Enable(1);
}

void DEC_Off(void) { DEC_Enable(0); }

/* 0 -> 7 순차 점등 */
void Demo_Count(uint32_t ms)
{
  for (uint8_t i = 0; i < 8; i++) { DEC_On(i); HAL_Delay(ms); }
}

/* 나이트라이더 (0->7->1) */
void Demo_Knight(uint32_t ms)
{
  for (uint8_t i = 0; i < 8; i++) { DEC_On(i); HAL_Delay(ms); }
  for (uint8_t i = 6; i > 0; i--) { DEC_On(i); HAL_Delay(ms); }
}

/* 8비트 패턴을 시분할(멀티플렉싱)로 표시. 비트0 = LED0 ... 비트7 = LED7 */
void Demo_Pattern(uint8_t pattern, uint32_t hold_ms)
{
  uint32_t t0 = HAL_GetTick();
  do {
    for (uint8_t i = 0; i < 8; i++) {
      if (pattern & (1u << i)) DEC_On(i);
      else                     DEC_Off();
      HAL_Delay(1);
    }
  } while (HAL_GetTick() - t0 < hold_ms);
  DEC_Off();
}

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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Demo_Count(200);          /* 0 -> 7 순차 */
    Demo_Knight(70);          /* 나이트라이더 */
    Demo_Knight(70);

    Demo_Pattern(0xAA, 1500); /* 10101010 */
    Demo_Pattern(0x0F, 1500); /* 00001111 */
    Demo_Pattern(0xFF, 1500); /* 전부 켜진 것처럼 보임 */

    DEC_Off();
    HAL_Delay(500);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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

/* USER CODE BEGIN 4 */

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
