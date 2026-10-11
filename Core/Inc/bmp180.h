#ifndef INC_BMP180_H_
#define INC_BMP180_H_

#include "main.h"
#include <stdint.h>

#define BMP180_ADDR        0xEE
#define BMP180_ADDR_FAULT  0xA0   /* 고장 주입용: 존재하지 않는 주소 */

extern volatile uint8_t fault_i2c; /* 1이면 잘못된 주소로 통신 -> 강제 실패 */

HAL_StatusTypeDef BMP180_Init(void);                     /* ID 확인 + 보정값 읽기 */
HAL_StatusTypeDef BMP180_ReadTemp(int32_t *temp_x10);    /* 0.1°C 단위 */
void              BMP180_Recover(void);                  /* I2C DeInit/Init */

#endif /* INC_BMP180_H_ */
