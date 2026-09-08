# ls138-counter

## 보드
- NUCLEO-F401RE (STM32F401RET6)

## 빌드 / 플래시
- 빌드: `cmake --build --preset Debug`
- 플래시: `STM32_Programmer_CLI -c port=SWD -w build\Debug\ls138-counter.elf -rst`

## 배선
- PA10 → 74LS138 핀1 (A)
- PB3 → 74LS138 핀2 (B)
- PB5 → 74LS138 핀3 (C)
- PB4 → 택트 스위치 (내부 풀업, 눌림 = LOW)
- 74LS138 핀6 (G1) = 5V 직결
- 74LS138 핀4 (/G2A) = GND 직결
- 74LS138 핀5 (/G2B) = GND 직결 (항상 활성)
- 74LS138 출력 Y0~Y7 (핀15,14,13,12,11,10,9,7) 각각 470Ω + LED → 5V 레일

## 규칙
- `USER CODE BEGIN` / `USER CODE END` 구역 바깥 수정 금지
- `.ioc` 파일과 `Drivers` 폴더 수정 금지
- 코드 수정 후 반드시 위 빌드 명령으로 직접 빌드해서 에러 확인할 것
- `HAL_Delay` 대신 `HAL_GetTick` 기반 비블로킹 타이밍 사용
