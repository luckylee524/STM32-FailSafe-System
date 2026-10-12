# STM32 Fail-safe Sensor/Actuator System

> **Embedded Linux + STM32 2-Node 기반 Fail-safe Sensor/Actuator System**을 목표로 하는 임베디드 개인 프로젝트입니다.
> 현재는 STM32 단일 노드에서 센서 읽기, 상태머신(FSM), Watchdog 기반 Fail-safe까지 구현했습니다.

## 개발 환경

- **MCU**: STM32F103C8T6 (Blue Pill), HSI 8MHz (PLL 미사용)
- **IDE / 라이브러리**: STM32CubeIDE, STM32 HAL
- **센서**: BMP180 (I2C, 7bit 주소 `0x77`)
- **PC 통신**: USB-TTL, PuTTY (115200 8N1)

## 핀 맵

- `PA0`: 제어 출력 (PWM 실습 시 TIM2_CH1)
- `PA1`: 버튼 입력 (EXTI1, Falling, Pull-up)
- `PA2`: 상태 LED (TIM2 인터럽트로 토글)
- `PA9` / `PA10`: USART1 TX / RX
- `PB6` / `PB7`: I2C1 SCL / SDA (BMP180)

## 로드맵 진행 현황

| <sub>주차</sub> | <sub>주제</sub> | <sub>핵심 개념</sub> | <sub>구현 내용</sub> |
|:-:|:-:|---|---|
| <sub>1</sub> | <sub>환경 구축</sub> | <sub>GPIO, Register</sub> | <sub>LED 토글(500/1000ms), 버튼 입력</sub> |
| <sub>2</sub> | <sub>기초</sub> | <sub>Interrupt, Timer</sub> | <sub>Polling vs EXTI, TIM2 0.5초 LED</sub> |
| <sub>3</sub> | <sub>기초</sub> | <sub>PWM</sub> | <sub>1kHz PWM, Duty 10/50/90%</sub> |
| <sub>4</sub> | <sub>통신</sub> | <sub>UART</sub> | <sub>PC ↔ STM32 CLI</sub> |
| <sub>5</sub> | <sub>데이터 전송</sub> | <sub>DMA</sub> | <sub>Blocking/IT/DMA 성능 비교</sub> |
| <sub>6</sub> | <sub>센서</sub> | <sub>I2C</sub> | <sub>BMP180 온도 측정, UART 출력</sub> |
| <sub>7</sub> | <sub>Firmware 구조</sub> | <sub>FSM, Watchdog</sub> | <sub>상태머신, IWDG, 고장 주입</sub> |

## 주차별 상세

### 1주차: GPIO / LED 토글
- 토글 방식으로 LED 제어, 딜레이를 500ms / 1000ms로 바꿔가며 점멸 주기 변화 확인

### 2주차: Interrupt / Timer
- 버튼 Polling: `HAL_GPIO_ReadPin()`으로 PA1을 읽어 PA0 제어
- 버튼 Interrupt: PA1 EXTI1(Falling, Pull-up), `HAL_GetTick()`으로 50ms 채터링 방지
- Timer Interrupt: TIM2 PSC=7999, ARR=499 → 8MHz / 8000 / 500 = **2Hz**
- 결과: 타이머 약 33초 동안 66회 증가(초당 2회)

### 3주차: PWM
- PSC=7, ARR=999 → 8MHz → 1MHz → **1kHz** PWM
- `__HAL_TIM_SET_COMPARE()`로 CCR 변경 (100 / 500 / 900)
- 10%와 50%는 밝기 차이가 뚜렷하지만 50%와 90%는 구분이 어려움 (사람 눈의 비선형 밝기 인지)

### 4주차: UART CLI
- USART1 115200 8N1, `HAL_UART_Receive_IT`로 1바이트씩 수신 후 에코
- `\r` / `\n`으로 줄 완성 판단, 빈 줄 무시, 버퍼 오버플로 방어(`rx_index < 99`)
- 명령어: `led on`, `led off`, `status`, 그 외 `unknown`

### 5주차: UART 전송 방식 비교 (1,000바이트)

| 방식 | 함수 반환 | 전송 완료 | 완료 시간 | 전송 중 loop |
|---|---|---|---|---|
| Blocking | 완료 후 반환 | 690,288 cycles | 86,286µs | 0 |
| Interrupt | 469 cycles | 690,405 cycles | 86,300µs | 29,032 |
| DMA | 356 cycles | 690,537 cycles | 86,317µs | 40,235 |

- 완료 시간은 UART 보레이트가 결정하므로 세 방식 모두 약 86.3ms
- DMA는 Interrupt보다 CPU 여유(loop 횟수)가 약 39% 많음
- 측정은 각 1회이므로 경향 확인용

### 6주차: I2C / BMP180
- I2C1 Standard Mode 100kHz, HAL 주소 `0xEE` (`0x77 << 1`)
- `HAL_I2C_IsDeviceReady()`로 ACK 확인, Chip ID `0xD0` → `0x55`
- 보정 계수 `0xAA~0xBF` 22바이트(AC1~MD 11개) 읽기, 데이터시트 공식으로 온도 계산
- 결과: `Temperature: 27.2 C` 출력 (실내 범위)

### 7주차: FSM / Watchdog (Fail-safe)

```
INIT ──► NORMAL ──(센서 오류)──► ERROR ──(200ms 재시도, 연속 3회 실패)──► SAFE
            ▲                                                              │
            └──────────── reset 명령 + 센서 응답 확인 ◄──────────────────────┘
```

- **ERROR**: 200ms 간격 재시도, 연속 3회 실패 시 SAFE
- **SAFE**: PA0 LOW 고정, PA2 100ms 점멸, `led on/off` 및 버튼 입력 차단, 자동 복구 없음
- **IWDG**: Prescaler 32, Reload 1250 (약 1초), 메인 루프 끝에서만 refresh
- 부팅 시 RCC 리셋 플래그로 리셋 원인 출력 (IWDG / SOFTWARE / POWER-ON / NRST PIN)
- I2C 타임아웃 1000ms → 20ms로 줄여 워치독과 충돌 방지

**고장 주입 명령 (재빌드 불필요)**

| 명령 | 동작 |
|---|---|
| `fault i2c` | 잘못된 I2C 주소(`0xA0`)로 통신해 강제 실패 |
| `fault clear` | 고장 해제 |
| `hang` | 무한루프로 워치독 리셋 유도 |
| `reset` | SAFE 복구 시도 (센서 응답 확인 후 NORMAL) |

**테스트 결과 (센서 읽기 주기 3000ms)**

| 실험 | 결과 |
|---|---|
| 코드 고장 주입 | NORMAL → ERROR → SAFE 확인 |
| 실제 SDA 단선 | NORMAL → ERROR → SAFE 확인 |
| ERROR → SAFE 소요 | 약 0.4초 (200ms × 2회), 주입/단선 일치 |
| SAFE 복구 | 고장 잔존 시 거부, 해제 후 `reset`으로 NORMAL 복귀 |
| IWDG 리셋 | `hang` 후 약 1초 내 재부팅, `Reset cause: IWDG` 출력 |

## 코드 구조

```
Core/
├── Inc/
│   ├── bmp180.h      # BMP180 센서 드라이버
│   └── fsm.h         # 상태머신, 명령 처리
└── Src/
    ├── main.c        # FSM_Init() / FSM_Run() 호출만 유지
    ├── bmp180.c
    └── fsm.c
```

- 모든 사용자 코드는 `USER CODE BEGIN/END` 영역 안에 작성하여 CubeMX 재생성 시에도 유지됩니다.

## 빌드 및 실행

1. STM32CubeIDE에서 프로젝트 Import
2. Build 후 ST-Link로 다운로드
3. USB-TTL(PA9/PA10) 연결 후 PuTTY 접속 (115200, 8N1)
4. 부팅 메시지와 `[tick] STATE A -> B` 로그 확인

## 다음 단계

- [ ] 8주차 이후 로드맵 (2-Node 구성, Embedded Linux 연동)
- [ ] 시스템 구성도 / State Diagram 이미지 추가
- [ ] 동작 영상 / 터미널 캡처 추가
