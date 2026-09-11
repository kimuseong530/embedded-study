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
#define DEC_CHANNEL_COUNT    8u   /* 74LS138은 3비트(A,B,C) 입력으로 채널 0~7(Y0~Y7)을 선택 */
#define BUTTON_DEBOUNCE_MS   20u  /* 버튼 채터링 방지용 디바운스 시간 (15~20ms 권장) */
#define LED_BLINK_ON_MS      120u /* 채널 표시용 LD2 점멸 ON 구간 길이 */
#define LED_BLINK_OFF_MS     120u /* 채널 표시용 LD2 점멸 OFF 구간 길이 */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint8_t s_currentChannel = 0; /* 현재 선택된 74LS138 채널 번호 (0~7) */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void DEC_Init(void);
static void DEC_Select(uint8_t channel);
static void DEC_Enable(uint8_t on);
static void LED_BlinkCount(uint8_t count);
static uint8_t Button_PollPressed(void);

#if 0
/* LED(+저항) 장착 후 테스트용 데모 함수 (기본 빌드에서는 컴파일되지 않음) */
static void Demo_SequentialScan(void);
static void Demo_KnightRider(void);
static void Demo_PatternDisplay(void);
#endif
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  74LS138 디코더 제어 모듈 초기화
  *         채널을 0번으로 선택하고 G1(출력 허가)을 HIGH로 올려 동작을 시작한다.
  *         G2A, G2B는 보드 배선상 GND에 고정되어 있어 소프트웨어 제어가 필요 없다.
  * @retval None
  */
static void DEC_Init(void)
{
  DEC_Select(0);
  DEC_Enable(1);
}

/**
  * @brief  74LS138의 A/B/C 셀렉트 라인에 채널 번호를 2진수로 출력한다.
  *         A = LSB (PA10), B = 중간 비트 (PB10), C = MSB (PA8)
  *         예) channel = 5(0b101) -> A=1, B=0, C=1 -> Y5 활성화
  * @param  channel 선택할 채널 번호 (0~7). 범위를 벗어나면 하위 3비트만 사용된다.
  * @retval None
  */
static void DEC_Select(uint8_t channel)
{
  channel &= 0x07u;

  HAL_GPIO_WritePin(A_1_GPIO_Port, A_1_Pin, (channel & 0x01u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(B_2_GPIO_Port, B_2_Pin, (channel & 0x02u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(C_3_GPIO_Port, C_3_Pin, (channel & 0x04u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
  * @brief  74LS138의 G1(출력 허가, active-high) 핀을 제어한다.
  *         G1이 LOW인 동안에는 A/B/C 값과 무관하게 모든 출력(Y0~Y7)이 비활성 상태(HIGH)이다.
  *         이 프로젝트에서는 항상 HIGH로 유지해 출력을 상시 허가한다.
  * @param  on 0이 아니면 G1 = HIGH(허가), 0이면 G1 = LOW(차단)
  * @retval None
  */
static void DEC_Enable(uint8_t on)
{
  HAL_GPIO_WritePin(G1_GPIO_Port, G1_Pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
  * @brief  보드 내장 LED(LD2, PA5)를 지정한 횟수만큼 짧게 깜빡인다.
  *         채널이 바뀔 때마다 "채널번호 + 1"번 호출하여 현재 채널을 시각적으로 확인한다.
  *         (예: 채널 0 선택 시 1번, 채널 7 선택 시 8번 깜빡임)
  * @param  count 깜빡일 횟수
  * @retval None
  */
static void LED_BlinkCount(uint8_t count)
{
  for (uint8_t i = 0; i < count; i++)
  {
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    HAL_Delay(LED_BLINK_ON_MS);
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    HAL_Delay(LED_BLINK_OFF_MS);
  }
}

/**
  * @brief  B1(PC13) 버튼을 폴링 방식으로 읽어 "눌림 엣지"(안 눌림->눌림, HIGH->LOW)를
  *         디바운스 처리 후 1회만 감지한다. 인터럽트(EXTI)는 사용하지 않는다.
  *         상태가 바뀐 순간에만 BUTTON_DEBOUNCE_MS 만큼 대기 후 재확인하는 방식으로
  *         채터링을 걸러낸다.
  * @retval 1: 이번 호출에서 새로운 "눌림" 엣지가 감지됨 / 0: 변화 없음
  */
static uint8_t Button_PollPressed(void)
{
  static GPIO_PinState lastStable = GPIO_PIN_SET; /* 안 눌림(HIGH)이 기본 상태 */
  uint8_t pressedEdge = 0;
  GPIO_PinState raw = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

  if (raw != lastStable)
  {
    HAL_Delay(BUTTON_DEBOUNCE_MS); /* 채터링 방지: 짧게 대기 후 재확인 */
    raw = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

    if (raw != lastStable)
    {
      if (raw == GPIO_PIN_RESET) /* HIGH -> LOW 로 안정적으로 바뀐 경우만 "눌림"으로 처리 */
      {
        pressedEdge = 1;
      }
      lastStable = raw;
    }
  }

  return pressedEdge;
}

#if 0
/* ==========================================================================
 * 데모 함수 모음 (기본 빌드에는 포함되지 않음, #if 1로 바꾸고 main()에서 호출)
 * LED 8개 + 470Ω 이상 저항 8개를 74LS138의 Y0~Y7 각 핀에 연결한 뒤 테스트할 것.
 *
 * 주의(하드웨어 제약):
 *  - 74LS138의 출력(Y0~Y7)은 5V(TTL) 레벨이므로 STM32 GPIO 입력으로 절대
 *    되돌리지 말 것 (STM32F401 GPIO는 5V 내성이 없어 손상 위험이 있음).
 *  - 출력 전류는 74LS138의 sink 한계(채널당 약 8mA)를 넘지 않도록 설계할 것.
 *    LED 추가 시 저항은 470Ω 이상을 권장 (5V 공급, LED Vf~2V 가정 시
 *    (5V - 2V) / 470Ω ≈ 6.4mA 로 8mA 이내).
 *  - 74LS138은 디먼서(demux) 구조라 활성화된 출력은 한 번에 하나뿐이다.
 *    따라서 아래 "패턴 표시"는 여러 채널을 순서대로 빠르게 전환하는 방식이다.
 * ========================================================================== */

/**
  * @brief  채널 0 -> 1 -> ... -> 7 -> 0 순으로 순차 점등하는 데모
  * @retval None
  */
static void Demo_SequentialScan(void)
{
  for (uint8_t ch = 0; ch < DEC_CHANNEL_COUNT; ch++)
  {
    DEC_Select(ch);
    HAL_Delay(200);
  }
}

/**
  * @brief  0 -> 7 -> 0 방향으로 왕복하며 점등하는 "나이트라이더" 스타일 데모
  * @retval None
  */
static void Demo_KnightRider(void)
{
  for (uint8_t ch = 0; ch < DEC_CHANNEL_COUNT; ch++)
  {
    DEC_Select(ch);
    HAL_Delay(150);
  }
  for (int8_t ch = DEC_CHANNEL_COUNT - 2; ch > 0; ch--)
  {
    DEC_Select((uint8_t)ch);
    HAL_Delay(150);
  }
}

/**
  * @brief  임의로 정한 순서(패턴)로 채널을 표시하는 데모
  * @retval None
  */
static void Demo_PatternDisplay(void)
{
  static const uint8_t pattern[] = {0, 2, 4, 6, 1, 3, 5, 7};

  for (uint8_t i = 0; i < sizeof(pattern) / sizeof(pattern[0]); i++)
  {
    DEC_Select(pattern[i]);
    HAL_Delay(250);
  }
}
#endif /* 0 : 데모 함수 모음 끝 */

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
  DEC_Init(); /* 채널 0 선택 + G1 HIGH(출력 허가) */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (Button_PollPressed())
    {
      /* 버튼을 누를 때마다 채널을 0->1->...->7->0 순으로 한 칸씩 이동 */
      s_currentChannel = (uint8_t)((s_currentChannel + 1u) % DEC_CHANNEL_COUNT);
      DEC_Select(s_currentChannel);

      /* 채널 변경 직후 "채널번호 + 1"번 LD2 깜빡임으로 현재 채널 표시 */
      LED_BlinkCount((uint8_t)(s_currentChannel + 1u));
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
