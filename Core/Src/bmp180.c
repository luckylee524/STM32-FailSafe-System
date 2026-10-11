#include "bmp180.h"

extern I2C_HandleTypeDef hi2c1;

#define I2C_TO_MS 20

volatile uint8_t fault_i2c = 0;

static int16_t  AC1, AC2, AC3, B1, B2, MB, MC, MD;
static uint16_t AC4, AC5, AC6;
static uint8_t  calib_ok = 0;

static uint16_t Addr(void)
{
    return fault_i2c ? BMP180_ADDR_FAULT : BMP180_ADDR;
}

HAL_StatusTypeDef BMP180_Init(void)
{
    uint8_t id = 0, c[22];
    HAL_StatusTypeDef r;

    calib_ok = 0;

    r = HAL_I2C_Mem_Read(&hi2c1, Addr(), 0xD0, I2C_MEMADD_SIZE_8BIT, &id, 1, I2C_TO_MS);
    if (r != HAL_OK) return r;
    if (id != 0x55)  return HAL_ERROR;

    r = HAL_I2C_Mem_Read(&hi2c1, Addr(), 0xAA, I2C_MEMADD_SIZE_8BIT, c, 22, I2C_TO_MS);
    if (r != HAL_OK) return r;

    /* 보정값이 0x0000 / 0xFFFF 이면 비정상 */
    for (int i = 0; i < 22; i += 2) {
        uint16_t w = (uint16_t)((c[i] << 8) | c[i + 1]);
        if (w == 0x0000 || w == 0xFFFF) return HAL_ERROR;
    }

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
    (void)AC1; (void)AC2; (void)AC3; (void)AC4; (void)B1; (void)B2; (void)MB;

    calib_ok = 1;
    return HAL_OK;
}

HAL_StatusTypeDef BMP180_ReadTemp(int32_t *temp_x10)
{
    uint8_t cmd = 0x2E, d[2];
    HAL_StatusTypeDef r;

    if (!calib_ok) return HAL_ERROR;

    r = HAL_I2C_Mem_Write(&hi2c1, Addr(), 0xF4, I2C_MEMADD_SIZE_8BIT, &cmd, 1, I2C_TO_MS);
    if (r != HAL_OK) return r;

    HAL_Delay(5);

    r = HAL_I2C_Mem_Read(&hi2c1, Addr(), 0xF6, I2C_MEMADD_SIZE_8BIT, d, 2, I2C_TO_MS);
    if (r != HAL_OK) return r;

    int32_t ut  = (int32_t)((d[0] << 8) | d[1]);
    int32_t x1  = ((ut - (int32_t)AC6) * (int32_t)AC5) >> 15;
    int32_t den = x1 + (int32_t)MD;
    if (den == 0) return HAL_ERROR;
    int32_t x2  = ((int32_t)MC << 11) / den;
    int32_t b5  = x1 + x2;

    *temp_x10 = (b5 + 8) >> 4;
    return HAL_OK;
}

void BMP180_Recover(void)
{
    HAL_I2C_DeInit(&hi2c1);
    HAL_I2C_Init(&hi2c1);     /* hi2c1.Init 값은 그대로 유지됨 */
}
