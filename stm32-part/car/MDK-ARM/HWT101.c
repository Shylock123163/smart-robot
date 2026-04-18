#include "HWT101.h"
#include "i2c.h"
#include "wit_c_sdk.h"

#define ACC_UPDATE         0x01
#define GYRO_UPDATE        0x02
#define ANGLE_UPDATE       0x04
#define MAG_UPDATE         0x08
#define READ_UPDATE        0x80
#define HWT101_I2C_TIMEOUT 100U

static volatile char s_cDataUpdate = 0;
float fAcc[3], fGyro[3], fAngle[3];

static void CopeSensorData(uint32_t uiReg, uint32_t uiRegNum);
static void AutoScanSensor(void);
static void delay_ms(uint16_t ucMs);

static void delay_ms(uint16_t ucMs)
{
    HAL_Delay(ucMs);
}

int32_t IICreadBytes(uint8_t dev, uint8_t reg, uint8_t *data, uint32_t length)
{
    if (HAL_I2C_Mem_Read(&hi2c1,
                         dev,
                         reg,
                         I2C_MEMADD_SIZE_8BIT,
                         data,
                         (uint16_t)length,
                         HWT101_I2C_TIMEOUT) != HAL_OK)
    {
        return 0;
    }

    return 1;
}

int32_t IICwriteBytes(uint8_t dev, uint8_t reg, uint8_t *data, uint32_t length)
{
    if (HAL_I2C_Mem_Write(&hi2c1,
                          dev,
                          reg,
                          I2C_MEMADD_SIZE_8BIT,
                          data,
                          (uint16_t)length,
                          HWT101_I2C_TIMEOUT) != HAL_OK)
    {
        return 0;
    }

    return 1;
}

static void CopeSensorData(uint32_t uiReg, uint32_t uiRegNum)
{
    int i;

    for (i = 0; i < uiRegNum; i++)
    {
        switch (uiReg)
        {
            case AZ:
                s_cDataUpdate |= ACC_UPDATE;
                break;
            case GZ:
                s_cDataUpdate |= GYRO_UPDATE;
                break;
            case HZ:
                s_cDataUpdate |= MAG_UPDATE;
                break;
            case Yaw:
                s_cDataUpdate |= ANGLE_UPDATE;
                break;
            default:
                s_cDataUpdate |= READ_UPDATE;
                break;
        }
        uiReg++;
    }
}

static void AutoScanSensor(void)
{
    int i;
    int iRetry;

    for (i = 0; i < 0x7F; i++)
    {
        WitInit(WIT_PROTOCOL_I2C, i);
        iRetry = 2;
        do
        {
            s_cDataUpdate = 0;
            WitReadReg(AX, 3);
            delay_ms(5);
            if (s_cDataUpdate != 0)
            {
                return;
            }
            iRetry--;
        } while (iRetry);
    }
}

void HWT101_Init(void)
{
    WitInit(WIT_PROTOCOL_I2C, 0x50);
    WitI2cFuncRegister(IICwriteBytes, IICreadBytes);
    WitRegisterCallBack(CopeSensorData);
    WitDelayMsRegister(delay_ms);
    AutoScanSensor();
}

void HWT101_GetValue(void)
{
    int i;

    WitReadReg(AX, 12);

    if (s_cDataUpdate)
    {
        for (i = 0; i < 3; i++)
        {
            fAcc[i] = sReg[AX + i] / 32768.0f * 16.0f;
            fGyro[i] = sReg[GX + i] / 32768.0f * 2000.0f;
            fAngle[i] = sReg[Roll + i] / 32768.0f * 180.0f;
        }
        if (s_cDataUpdate & ACC_UPDATE)
        {
            s_cDataUpdate &= (char)(~ACC_UPDATE);
        }
        if (s_cDataUpdate & GYRO_UPDATE)
        {
            s_cDataUpdate &= (char)(~GYRO_UPDATE);
        }
        if (s_cDataUpdate & ANGLE_UPDATE)
        {
            s_cDataUpdate &= (char)(~ANGLE_UPDATE);
        }
        if (s_cDataUpdate & MAG_UPDATE)
        {
            s_cDataUpdate &= (char)(~MAG_UPDATE);
        }
    }
}