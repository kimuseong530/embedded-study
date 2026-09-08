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
#include <string.h>   /* BTN3 매핑 테스트에서 UART 문자열 길이 계산(strlen)에 사용 */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* 버튼 1개의 디바운스 상태를 담는 구조체.
   port/pin  : 이 버튼이 연결된 GPIO (예: BTN1_GPIO_Port, BTN1_Pin)
   lastReading    : 방금 전 루프에서 읽은 raw 핀 값 (튀는 값 포함, 아직 확정 아님)
   stableState    : 디바운스를 통과해 "확정"된 값 (실제로 눌렸는지/안 눌렸는지)
   lastChangeTick : lastReading이 마지막으로 바뀐 시각(ms) - 여기서부터 시간을 재서
                    BUTTON_DEBOUNCE_MS 이상 값이 안 변하면 진짜 상태로 인정 */
typedef struct
{
  GPIO_TypeDef *port;
  uint16_t pin;
  GPIO_PinState lastReading;
  GPIO_PinState stableState;
  uint32_t lastChangeTick;
} Button_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUTTON_DEBOUNCE_MS 20  /* 버튼 접점이 튀는(chattering) 시간을 걸러내기 위한 안정화 대기 시간 */

/* 1이면 부팅 직후 LED 배선 매핑 테스트(BTN3_Action)를 자동으로 한 번 실행한다.
   버튼을 누르지 않아도 되므로 BTN3 배선 여부와 무관하게 확인할 수 있다.
   배선 매핑 확인이 끝났으므로 0으로 꺼둠 - 다시 배선을 만지게 되면 1로 바꾸면 됨. */
#define RUN_MAPPING_TEST_AT_BOOT 0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* 버튼 4개 각각의 상태. 초기값 GPIO_PIN_SET(HIGH)은 "안 눌림" 가정이고,
   실제 초기값은 main() 시작 시 Button_Init()이 실제 핀을 읽어서 다시 세팅한다. */
static Button_t btn1 = { BTN1_GPIO_Port, BTN1_Pin, GPIO_PIN_SET, GPIO_PIN_SET, 0 };
static Button_t btn2 = { BTN2_GPIO_Port, BTN2_Pin, GPIO_PIN_SET, GPIO_PIN_SET, 0 };
static Button_t btn3 = { BTN3_GPIO_Port, BTN3_Pin, GPIO_PIN_SET, GPIO_PIN_SET, 0 };
static Button_t btn4 = { BTN4_GPIO_Port, BTN4_Pin, GPIO_PIN_SET, GPIO_PIN_SET, 0 };

static uint8_t ledIndex = 0;  /* 현재 74LS138이 선택 중인 LED 번호 (0~7) */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Button_Init(Button_t *btn);
static uint8_t Button_Update(Button_t *btn);
static void SetDecoderOutput(uint8_t index);

static void BTN1_Action(void);
static void BTN2_Action(void);
static void BTN3_Action(void);
static void BTN4_Action(void);
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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  /* 리셋 직후 전원/풀업이 안정되기 전 핀이 잠깐 튈 수 있으므로,
     "안 눌림"으로 무작정 가정하지 않고 지금 실제 핀 값을 읽어서
     그걸 기준(baseline) 상태로 저장한다. 이렇게 하면 리셋 직후의
     순간적인 노이즈를 "버튼이 눌렸다"고 오인하지 않는다. */
  Button_Init(&btn1);
  Button_Init(&btn2);
  Button_Init(&btn3);
  Button_Init(&btn4);

  SetDecoderOutput(ledIndex);  /* 시작 시 LED0(index 0)을 켠 상태로 초기화 */

  /* 부팅 배너: UART가 살아있는지 바로 확인하기 위한 신호.
     리셋하자마자 이 줄이 시리얼 모니터에 뜨면 UART/포트/보드레이트는 정상이라는 뜻. */
  {
    static const char banner[] = "\r\n=== 04-74ls138-4btn ready (115200 8N1) ===\r\n";
    HAL_UART_Transmit(&huart2, (const uint8_t *)banner, sizeof(banner) - 1, HAL_MAX_DELAY);
  }

#if RUN_MAPPING_TEST_AT_BOOT
  /* 배선 매핑 확인이 끝나면 RUN_MAPPING_TEST_AT_BOOT를 0으로 바꿔서 끄면 된다. */
  BTN3_Action();
#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* 매 루프마다 버튼 4개를 각각 폴링(polling)한다.
       Button_Update()가 1을 반환하는 순간(=눌리는 그 찰나)에만
       해당 버튼의 액션 함수를 딱 한 번 호출한다. */
    if (Button_Update(&btn1)) { BTN1_Action(); }
    if (Button_Update(&btn2)) { BTN2_Action(); }
    if (Button_Update(&btn3)) { BTN3_Action(); }
    if (Button_Update(&btn4)) { BTN4_Action(); }
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
/**
  * @brief  버튼의 디바운스 상태를 "현재 실제 핀 값"으로 초기화한다.
  *         전원 인가/리셋 직후의 과도(transient) 상태를 눌림으로 오인하지 않기 위함.
  */
static void Button_Init(Button_t *btn)
{
  GPIO_PinState reading = HAL_GPIO_ReadPin(btn->port, btn->pin);
  btn->lastReading = reading;      /* 지금 읽은 값을 그대로 "직전 값"으로 */
  btn->stableState = reading;      /* 지금 읽은 값을 그대로 "확정 값"으로 → 이벤트 발생 안 함 */
  btn->lastChangeTick = HAL_GetTick();
}

/**
  * @brief  버튼 핀 하나를 디바운스하며 폴링한다.
  * @retval 버튼이 눌리는(HIGH→LOW) 그 순간에만 1, 그 외에는 0.
  *
  * 동작 원리(소프트웨어 디바운스):
  *  1) 매 호출마다 현재 핀 값을 읽는다.
  *  2) 직전에 읽은 값(lastReading)과 다르면 "지금부터 다시 시간을 잰다"
  *     (lastChangeTick 갱신) - 버튼 접점이 튀는(chattering) 짧은 순간들을
  *     여기서 계속 리셋시켜 걸러낸다.
  *  3) 값이 BUTTON_DEBOUNCE_MS(20ms) 이상 안 변하고 유지되면 그제서야
  *     "진짜로 상태가 바뀌었다"고 확정(stableState 갱신)한다.
  *  4) 확정된 상태가 GPIO_PIN_RESET(LOW, 눌림)으로 바뀌는 그 순간에만
  *     pressedEvent를 1로 만든다. 즉 누르고 있는 동안 계속 1이 나오는 게
  *     아니라, 눌리는 찰나에 딱 한 번만 1이 나온다.
  */
static uint8_t Button_Update(Button_t *btn)
{
  GPIO_PinState reading = HAL_GPIO_ReadPin(btn->port, btn->pin);
  uint8_t pressedEvent = 0;

  if (reading != btn->lastReading)
  {
    btn->lastChangeTick = HAL_GetTick();
  }

  if ((HAL_GetTick() - btn->lastChangeTick) >= BUTTON_DEBOUNCE_MS)
  {
    if (reading != btn->stableState)
    {
      btn->stableState = reading;
      if (btn->stableState == GPIO_PIN_RESET)
      {
        pressedEvent = 1;
      }
    }
  }

  btn->lastReading = reading;
  return pressedEvent;
}

/**
  * @brief  74LS138의 A/B/C 선택 입력(PB3/PB5/PA10)에 index(0~7)를 3비트로
  *         실어서, 8개 출력(Y0~Y7) 중 하나만 활성화되게 한다.
  *
  * index를 2진수로 봤을 때:
  *   bit0(0x01) → DEC_A (74LS138의 A, 1번 핀)
  *   bit1(0x02) → DEC_B (74LS138의 B, 2번 핀)
  *   bit2(0x04) → DEC_C (74LS138의 C, 3번 핀)
  * 예) index=3(0b011) → A=1, B=1, C=0 → Y3만 LOW(활성)가 되어 LED3 켜짐.
  * G1(인에이블)은 항상 VCC에 물려있으므로 이 함수만으로 항상 하나의 LED가 켜진다.
  */
static void SetDecoderOutput(uint8_t index)
{
  HAL_GPIO_WritePin(DEC_A_GPIO_Port, DEC_A_Pin, (index & 0x01U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(DEC_B_GPIO_Port, DEC_B_Pin, (index & 0x02U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(DEC_C_GPIO_Port, DEC_C_Pin, (index & 0x04U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* TODO: BTN2~4 동작은 여기에 정의. 예) ledIndex를 갱신한 뒤 SetDecoderOutput(ledIndex) 호출 */

/* BTN1을 누르면 LED0부터 LED7까지 1초 간격으로 순서대로 하나씩 켠다.
   74LS138은 한 번에 출력 하나만 활성화되므로, 다음 LED가 켜지면 이전 LED는
   자동으로 꺼진다. for문이 끝날 때까지(총 8초) HAL_Delay로 대기하는 동안은
   메인 루프가 멈춰 있어 다른 버튼 입력은 그 사이에 받아들여지지 않는다. */
static void BTN1_Action(void)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    ledIndex = i;
    SetDecoderOutput(ledIndex);
    HAL_Delay(1000);
  }
}

/* BTN2를 누르면 BTN1과 같은 방식으로 LED0->LED7을 한 방향(편도)으로 켜는 동작을
   3번 반복한다. 매 스텝마다 대기 시간을 조금씩 줄여서(BTN2_DELAY_STEP_MS씩 감소,
   BTN2_MIN_DELAY_MS 밑으로는 안 내려감) 반복할수록 빨라지며, 속도는 3번 반복
   내내 리셋되지 않고 계속 가속된다. */
#define BTN2_INITIAL_DELAY_MS 1000
#define BTN2_DELAY_STEP_MS    40
#define BTN2_MIN_DELAY_MS     50
#define BTN2_REPEAT_COUNT     3

static void BTN2_Action(void)
{
  uint16_t delay = BTN2_INITIAL_DELAY_MS;

  for (uint8_t rep = 0; rep < BTN2_REPEAT_COUNT; rep++)
  {
    for (uint8_t i = 0; i <= 7; i++)
    {
      ledIndex = i;
      SetDecoderOutput(ledIndex);
      HAL_Delay(delay);
      /* 하한(BTN2_MIN_DELAY_MS)까지만 줄이고 그 아래로는 내려가지 않는다.
         한 스텝 더 빼면 하한 밑으로 떨어지는 경우엔 하한 값으로 고정. */
      if (delay >= BTN2_MIN_DELAY_MS + BTN2_DELAY_STEP_MS) { delay -= BTN2_DELAY_STEP_MS; }
      else { delay = BTN2_MIN_DELAY_MS; }
    }
  }
}

/* BTN3: LED 배선 매핑 확인용 테스트.
   index를 0부터 7까지 하나씩, 각각 BTN3_TEST_DWELL_MS(3초)씩 길게 켜두고
   동시에 UART(USART2, 115200 8N1, ST-Link 가상 COM 포트)로 현재 index와
   그에 대응하는 74LS138 출력/물리 핀 번호를 찍어준다.

   사용법: 시리얼 모니터를 열고 BTN3을 누른 뒤, 메시지가 바뀔 때마다
   실제로 어떤 위치의 LED가 켜지는지 순서대로 적어두면 된다.
   - 화면 순서대로 LED가 한 칸씩 이동하면 → 배선 정상, 코드도 정상
   - 화면은 0,1,2...로 잘 올라가는데 LED가 여기저기 튀면 → 배선 순서 문제
   - 화면은 잘 나오는데 LED가 아예 안 켜지는 index가 있으면 → 그 출력 핀 배선/LED 불량 */
#define BTN3_TEST_DWELL_MS 3000

/* index -> 74LS138 출력(Y)과 실제 칩 핀 번호. Y0~Y6은 15번에서 9번으로 내림차순이고
   Y7만 7번 핀이라 물리적 순서가 index 순서와 다르다는 점에 주의. */
static const char *const decoderPinLabel[8] = {
  "index=0 -> Y0 (74LS138 pin 15)\r\n",
  "index=1 -> Y1 (74LS138 pin 14)\r\n",
  "index=2 -> Y2 (74LS138 pin 13)\r\n",
  "index=3 -> Y3 (74LS138 pin 12)\r\n",
  "index=4 -> Y4 (74LS138 pin 11)\r\n",
  "index=5 -> Y5 (74LS138 pin 10)\r\n",
  "index=6 -> Y6 (74LS138 pin 9)\r\n",
  "index=7 -> Y7 (74LS138 pin 7)\r\n",
};

static void BTN3_Action(void)
{
  static const char header[] = "\r\n--- LED mapping test (3s each) ---\r\n";

  HAL_UART_Transmit(&huart2, (const uint8_t *)header, sizeof(header) - 1, HAL_MAX_DELAY);

  for (uint8_t i = 0; i < 8; i++)
  {
    ledIndex = i;
    SetDecoderOutput(ledIndex);
    HAL_UART_Transmit(&huart2, (const uint8_t *)decoderPinLabel[i],
                      strlen(decoderPinLabel[i]), HAL_MAX_DELAY);
    HAL_Delay(BTN3_TEST_DWELL_MS);
  }
}

/* 아직 미구현 - 추후 동작 내용 채워 넣을 자리 */
static void BTN4_Action(void)
{
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
