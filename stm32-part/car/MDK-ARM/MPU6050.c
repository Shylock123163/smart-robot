#include "mpu6050.h"

extern I2C_HandleTypeDef hi2c2;

/* 航向角（Z轴积分）*/
static float g_yaw = 0.0f;

/**
 * @brief 写寄存器
 */
static HAL_StatusTypeDef MPU6050_WriteReg(uint8_t reg, uint8_t data)
{
    return HAL_I2C_Mem_Write(&hi2c2,
                              MPU6050_ADDR,
                              reg, I2C_MEMADD_SIZE_8BIT,
                              &data, 1,
                              100);
}

/**
 * @brief 读寄存器
 */
static HAL_StatusTypeDef MPU6050_ReadReg(uint8_t reg,
                                          uint8_t *buf,
                                          uint16_t len)
{
    return HAL_I2C_Mem_Read(&hi2c2,
                             MPU6050_ADDR,
                             reg, I2C_MEMADD_SIZE_8BIT,
                             buf, len,
                             100);
}

/**
 * @brief 初始化MPU6050
 * @retval 0=成功 1=失败（I2C通信异常或设备不存在）
 */
uint8_t MPU6050_Init(void)
{
    uint8_t who_am_i = 0;

    /* 等待I2C稳定 */
    HAL_Delay(100);

    /* 检查设备ID，正常应返回0x68 */
    if(MPU6050_ReadReg(MPU6050_WHO_AM_I, &who_am_i, 1) != HAL_OK)
        return 1;
    if(who_am_i != 0x68)
        return 1;

    /* 解除睡眠，使用内部8MHz时钟 */
    if(MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x00) != HAL_OK)
        return 1;

    HAL_Delay(10);

    /* 陀螺仪量程设为±500°/s（FS_SEL=1）*/
    if(MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x08) != HAL_OK)
        return 1;

    g_yaw = 0.0f;

    return 0;  // 成功
}

/**
 * @brief 读取Z轴角速度（°/s）
 * @retval Z轴角速度，正值为左转，负值为右转
 */
float MPU6050_GetGyroZ(void)
{
    uint8_t buf[2];
    int16_t raw;

    if(MPU6050_ReadReg(MPU6050_GYRO_ZOUT_H, buf, 2) != HAL_OK)
        return 0.0f;

    raw = (int16_t)((buf[0] << 8) | buf[1]);

    /* 去除零偏（静止时陀螺仪不为0，需要减去零偏）*/
    return (float)raw / GYRO_SENSITIVITY;
}

/**
 * @brief 更新航向角（每10ms调用一次）
 * @note  在IMUTask的osDelay(10)之前调用
 */
void MPU6050_UpdateYaw(void)
{
    float gyro_z = MPU6050_GetGyroZ();

    /* 死区处理：角速度小于0.1°/s认为静止，不积分（减少漂移）*/
    if(gyro_z > 0.1f || gyro_z < -0.1f)
    {
        g_yaw += gyro_z * 0.01f;  // 积分：角速度 × 时间(10ms=0.01s)
    }
}

/**
 * @brief 重置航向角为0
 * @note  小车启动进入沙发前调用，以当前方向为基准
 */
void MPU6050_ResetYaw(void)
{
    g_yaw = 0.0f;
}

/**
 * @brief 获取当前航向角
 * @retval 航向角（°），以启动时方向为0度基准
 */
float MPU6050_GetYaw(void)
{
    return g_yaw;
}