#include "fsm.h"
#include "bmp180.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* ---- IWDG: CubeMX에서 켜기 전에도 컴파일되도록 처리 ---- */
#ifdef HAL_IWDG_MODULE_ENABLED
extern IWDG_HandleTypeDef hiwdg;
#define WDG_FEED() HAL_IWDG_Refresh(&hiwdg)
#else
#define WDG_FEED() ((void)0)
#endif

extern UART_HandleTypeDef huart1;

/* ---- 설정값 ---- */
#define SENSOR_PERIOD_MS  3000   /* NORMAL 읽기 주기 */
#define RETRY_PERIOD_MS   200   /* ERROR 재시도 주기 */
#define FAIL_LIMIT        3     /* 연속 실패 N회 -> SAFE */

/* ---- 상태 ---- */
static FsmState_t state = ST_INIT;
static const char *state_name[] = { "INIT", "NORMAL", "ERROR", "SAFE" };
static uint8_t  fail_cnt = 0;
static uint32_t next_read = 0, next_try = 0;
static uint32_t t_last_ok = 0, t_fault = 0;

/* ---- UART 수신 ---- */
static uint8_t rx_byte;
static uint8_t rx_buf[100];
static volatile uint8_t rx_index = 0;
static volatile uint8_t line_ready = 0;

/* ---- 로그 ---- */
static void Log(const char *fmt, ...)
{
    char buf[128];
    int n = snprintf(buf, sizeof(buf), "[%lu] ", (unsigned long)HAL_GetTick());
    va_list ap;
    va_start(ap, fmt);
    n += vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);
    va_end(ap);
    if (n > (int)sizeof(buf) - 3) n = sizeof(buf) - 3;
    buf[n++] = '\r';
    buf[n++] = '\n';
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, n, 50);
}

static void LogTemp(int32_t t)
{
    Log("TEMP %s%ld.%ld C", (t < 0) ? "-" : "", labs(t) / 10, labs(t) % 10);
}

/* ---- 안전 출력 ---- */
static void ApplySafeOutputs(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);   /* 제어 출력 = LOW */
}

static void SetState(FsmState_t ns)
{
    uint32_t now = HAL_GetTick();
    uint32_t t_ref = fault_i2c ? t_fault : t_last_ok;

    Log("STATE %s -> %s", state_name[state], state_name[ns]);
    state = ns;

    if (ns == ST_ERROR) Log("  error_ms=%lu (기준: %s)", (unsigned long)(now - t_ref),
                            fault_i2c ? "fault cmd" : "last OK");
    if (ns == ST_SAFE) {
        ApplySafeOutputs();
        Log("  safe_ms=%lu (기준: %s)", (unsigned long)(now - t_ref),
            fault_i2c ? "fault cmd" : "last OK");
    }
}

/* PA2 = 상태 LED: NORMAL 1s / ERROR 300ms / SAFE 100ms 점멸 */
static void UpdateStatusLed(void)
{
    static uint32_t last = 0;
    uint32_t period = (state == ST_SAFE) ? 100 : (state == ST_ERROR) ? 300
                    : (state == ST_NORMAL) ? 1000 : 500;
    uint32_t now = HAL_GetTick();
    if (now - last >= period) {
        last = now;
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_2);
    }
}

/* ---- 명령 처리 ---- */
static void HandleCommand(const char *cmd)
{
    if (strcmp(cmd, "led on") == 0 || strcmp(cmd, "led off") == 0) {
        if (state == ST_SAFE) { Log("BLOCKED: SAFE 상태에서 제어 명령 차단"); return; }
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0,
            (strcmp(cmd, "led on") == 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        Log("OK %s", cmd);
    }
    else if (strcmp(cmd, "status") == 0) {
        Log("STATE=%s fail_cnt=%u LED=%s fault_i2c=%u", state_name[state], fail_cnt,
            HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) ? "ON" : "OFF", fault_i2c);
    }
    else if (strcmp(cmd, "fault i2c") == 0) {
        t_fault = HAL_GetTick();
        fault_i2c = 1;
        Log("FAULT INJECT: I2C wrong addr");
    }
    else if (strcmp(cmd, "fault clear") == 0) {
        fault_i2c = 0;
        Log("FAULT CLEARED");
    }
    else if (strcmp(cmd, "reset") == 0) {
        if (state != ST_SAFE) { Log("reset은 SAFE 상태에서만 사용"); return; }
        BMP180_Recover();
        if (BMP180_Init() == HAL_OK) {
            fail_cnt = 0;
            t_last_ok = HAL_GetTick();
            next_read = t_last_ok;
            SetState(ST_NORMAL);
        } else {
            Log("RECOVER FAIL: 센서 응답 없음, SAFE 유지");
        }
    }
    else if (strcmp(cmd, "hang") == 0) {
        Log("HANG: 무한루프 진입 (IWDG 리셋 대기)");
        while (1) { }          /* 의도적으로 feed 중단 */
    }
    else {
        Log("unknown: %s", cmd);
    }
}

/* ---- 공개 함수 ---- */
void FSM_Init(void)
{
    WDG_FEED();

    Log("==== BOOT ====");
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))      Log("Reset cause: IWDG");
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))  Log("Reset cause: SOFTWARE");
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))  Log("Reset cause: POWER-ON");
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))  Log("Reset cause: NRST PIN");
    else                                           Log("Reset cause: UNKNOWN");
    __HAL_RCC_CLEAR_RESET_FLAGS();

    ApplySafeOutputs();                 /* 부팅 직후도 안전 출력부터 */
    state = ST_INIT;
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

void FSM_Run(void)
{
    uint32_t now = HAL_GetTick();

    /* 1) UART 명령 */
    if (line_ready) {
        Log("> %s", (char *)rx_buf);
        HandleCommand((char *)rx_buf);
        rx_index = 0;
        line_ready = 0;
    }

    /* 2) FSM */
    switch (state) {
    case ST_INIT:
        if (BMP180_Init() == HAL_OK) {
            fail_cnt = 0;
            t_last_ok = now;
            next_read = now;
            SetState(ST_NORMAL);
        } else {
            fail_cnt = 1;
            next_try = now + RETRY_PERIOD_MS;
            Log("INIT fail (%u/%d)", fail_cnt, FAIL_LIMIT);
            SetState(ST_ERROR);
        }
        break;

    case ST_NORMAL:
        if ((int32_t)(now - next_read) >= 0) {
            int32_t t;
            next_read = now + SENSOR_PERIOD_MS;
            if (BMP180_ReadTemp(&t) == HAL_OK) {
                t_last_ok = HAL_GetTick();
                LogTemp(t);
            } else {
                fail_cnt = 1;
                next_try = now + RETRY_PERIOD_MS;
                Log("I2C read fail (%u/%d)", fail_cnt, FAIL_LIMIT);
                SetState(ST_ERROR);
            }
        }
        break;

    case ST_ERROR:
        if ((int32_t)(now - next_try) >= 0) {
            int32_t t;
            HAL_StatusTypeDef r;
            BMP180_Recover();                       /* BUSY 상태 대비 */
            r = BMP180_Init();
            if (r == HAL_OK) r = BMP180_ReadTemp(&t);

            if (r == HAL_OK) {
                fail_cnt = 0;
                t_last_ok = HAL_GetTick();
                next_read = t_last_ok + SENSOR_PERIOD_MS;
                LogTemp(t);
                SetState(ST_NORMAL);
            } else {
                fail_cnt++;
                Log("retry fail (%u/%d)", fail_cnt, FAIL_LIMIT);
                if (fail_cnt >= FAIL_LIMIT) SetState(ST_SAFE);
                else next_try = HAL_GetTick() + RETRY_PERIOD_MS;
            }
        }
        break;

    case ST_SAFE:
        ApplySafeOutputs();     /* EXTI 등으로 바뀌어도 매 바퀴 안전값으로 복원 */
        break;

    default:
        break;                  /* 비정상 상태값이면 아래 feed 안 함 */
    }

    UpdateStatusLed();

    /* 3) Watchdog feed: 메인 루프 끝에서만, 상태값이 유효할 때만 */
    if (state <= ST_SAFE) {
        WDG_FEED();
    }
}

/* ---- HAL 콜백 (main.c에서는 삭제할 것) ---- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
        if (line_ready) return;                       /* 처리 전 입력 무시 */

        HAL_UART_Transmit(&huart1, &rx_byte, 1, 10);  /* 에코 */

        if (rx_byte == '\n' || rx_byte == '\r') {
            if (rx_index > 0) {
                rx_buf[rx_index] = '\0';
                line_ready = 1;
            }
        } else if (rx_index < sizeof(rx_buf) - 1) {
            rx_buf[rx_index++] = rx_byte;
        }
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last_tick = 0;
    if (GPIO_Pin == GPIO_PIN_1) {
        uint32_t now = HAL_GetTick();
        if (state == ST_SAFE) return;                 /* SAFE에서는 버튼 제어도 차단 */
        if (now - last_tick > 50) {
            last_tick = now;
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);
        }
    }
}
