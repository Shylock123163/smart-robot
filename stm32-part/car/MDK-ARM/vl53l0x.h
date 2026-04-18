#ifndef __VL53L0X_H
#define __VL53L0X_H

#include "main.h"
#include "i2c.h"
#include <stdint.h>

/* ══════════════════════════════════════
   I2C 配置
   PB10 = I2C2_SCL
   PB11 = I2C2_SDA
   ══════════════════════════════════════ */
extern I2C_HandleTypeDef hi2c2;
#define VL53L0X_I2C    hi2c2

/* ══════════════════════════════════════
   XSHUT 引脚定义
   PF6  = 左侧传感器 XSHUT
   PF8  = 前方传感器 XSHUT
   PF10 = 右侧传感器 XSHUT
   ══════════════════════════════════════ */
#define XSHUT_L_PORT   GPIOF
#define XSHUT_L_PIN    GPIO_PIN_6
#define XSHUT_F_PORT   GPIOF
#define XSHUT_F_PIN    GPIO_PIN_8
#define XSHUT_R_PORT   GPIOF
#define XSHUT_R_PIN    GPIO_PIN_10

/* ══════════════════════════════════════
   传感器I2C地址（重新分配后）
   默认地址0x52（8位），分配后左移1位
   ══════════════════════════════════════ */
#define VL53L0X_DEFAULT_ADDR   0x52   // 出厂默认地址
#define VL53L0X_ADDR_LEFT      0x30   // 左侧传感器分配地址
#define VL53L0X_ADDR_FRONT     0x32   // 前方传感器分配地址
#define VL53L0X_ADDR_RIGHT     0x34   // 右侧传感器分配地址

/* 避障距离阈值（mm）*/
#define VL53L0X_AVOID_DIST     150

/* 寄存器地址定义 */
#define SYSRANGE_START                              0x00
#define SYSTEM_THRESH_HIGH                          0x0C
#define SYSTEM_THRESH_LOW                           0x0E
#define SYSTEM_SEQUENCE_CONFIG                      0x01
#define SYSTEM_RANGE_CONFIG                         0x09
#define SYSTEM_INTERMEASUREMENT_PERIOD              0x04
#define SYSTEM_INTERRUPT_CONFIG_GPIO                0x0A
#define GPIO_HV_MUX_ACTIVE_HIGH                     0x84
#define SYSTEM_INTERRUPT_CLEAR                      0x0B
#define RESULT_INTERRUPT_STATUS                     0x13
#define RESULT_RANGE_STATUS                         0x14
#define RESULT_CORE_AMBIENT_WINDOW_EVENTS_RTN       0xBC
#define RESULT_CORE_RANGING_TOTAL_EVENTS_RTN        0xC0
#define RESULT_CORE_AMBIENT_WINDOW_EVENTS_REF       0xD0
#define RESULT_CORE_RANGING_TOTAL_EVENTS_REF        0xD4
#define RESULT_PEAK_SIGNAL_RATE_REF                 0xB6
#define ALGO_PART_TO_PART_RANGE_OFFSET_MM           0x28
#define I2C_SLAVE_DEVICE_ADDRESS                    0x8A
#define MSRC_CONFIG_CONTROL                         0x60
#define PRE_RANGE_CONFIG_MIN_SNR                    0x27
#define PRE_RANGE_CONFIG_VALID_PHASE_LOW            0x56
#define PRE_RANGE_CONFIG_VALID_PHASE_HIGH           0x57
#define PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT          0x64
#define FINAL_RANGE_CONFIG_MIN_SNR                  0x67
#define FINAL_RANGE_CONFIG_VALID_PHASE_LOW          0x47
#define FINAL_RANGE_CONFIG_VALID_PHASE_HIGH         0x48
#define FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT 0x44
#define PRE_RANGE_CONFIG_SIGMA_THRESH_HI            0x61
#define PRE_RANGE_CONFIG_SIGMA_THRESH_LO            0x62
#define PRE_RANGE_CONFIG_VCSEL_PERIOD               0x50
#define PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI          0x51
#define PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO          0x52
#define SYSTEM_HISTOGRAM_BIN                        0x81
#define HISTOGRAM_CONFIG_INITIAL_PHASE_SELECT       0x33
#define HISTOGRAM_CONFIG_READOUT_CTRL               0x55
#define FINAL_RANGE_CONFIG_VCSEL_PERIOD             0x70
#define FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI        0x71
#define FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO        0x72
#define CROSSTALK_COMPENSATION_PEAK_RATE_MCPS       0x20
#define MSRC_CONFIG_TIMEOUT_MACROP                  0x46
#define SOFT_RESET_GO2_SOFT_RESET_N                 0xBF
#define IDENTIFICATION_MODEL_ID                     0xC0
#define IDENTIFICATION_REVISION_ID                  0xC2
#define OSC_CALIBRATE_VAL                           0xF8
#define GLOBAL_CONFIG_VCSEL_WIDTH                   0x32
#define GLOBAL_CONFIG_SPAD_ENABLES_REF_0            0xB0
#define GLOBAL_CONFIG_REF_EN_START_SELECT           0xB6
#define DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD         0x4E
#define DYNAMIC_SPAD_REF_EN_START_OFFSET            0x4F
#define POWER_MANAGEMENT_GO1_POWER_FORCE            0x80
#define VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV          0x89
#define ALGO_PHASECAL_LIM                           0x30
#define ALGO_PHASECAL_CONFIG_TIMEOUT                0x30

/* VCSEL周期类型枚举 */
typedef enum {
    VcselPeriodPreRange,
    VcselPeriodFinalRange
} vcselPeriodType;

/* 序列步骤使能结构体 */
typedef struct {
    uint8_t tcc, msrc, dss, pre_range, final_range;
} SequenceStepEnables;

/* 序列步骤超时结构体 */
typedef struct {
    uint16_t pre_range_vcsel_period_pclks;
    uint16_t final_range_vcsel_period_pclks;
    uint16_t msrc_dss_tcc_mclks;
    uint32_t msrc_dss_tcc_us;
    uint16_t pre_range_mclks;
    uint32_t pre_range_us;
    uint16_t final_range_mclks;
    uint32_t final_range_us;
} SequenceStepTimeouts;

/* 对外接口函数声明 */
uint8_t  VL53L0X_Init_All(void);
uint16_t VL53L0X_ReadDistance(uint8_t sensor_id);

/* 内部函数声明 */
void     VL53L0X_SetCurrentAddr(uint8_t addr);
void     VL53L0X_writeReg(uint8_t reg, uint8_t value);
void     VL53L0X_writeReg16Bit(uint8_t reg, uint16_t value);
void     VL53L0X_writeReg32Bit(uint8_t reg, uint32_t value);
uint8_t  VL53L0X_readReg(uint8_t reg);
uint16_t VL53L0X_readReg16Bit(uint8_t reg);
void     VL53L0X_writeMulti(uint8_t reg, uint8_t const *src, uint8_t count);
void     VL53L0X_readMulti(uint8_t reg, uint8_t *dst, uint8_t count);
uint8_t  VL53L0X_init(uint8_t io_2v8);
void     VL53L0X_setAddress(uint8_t new_addr);
void     VL53L0X_startContinuous(uint32_t period_ms);
uint16_t VL53L0X_readRangeContinuousMillimeters(void);
uint8_t  VL53L0X_setSignalRateLimit(float limit_Mcps);
uint8_t  VL53L0X_setMeasurementTimingBudget(uint32_t budget_us);
uint32_t VL53L0X_getMeasurementTimingBudget(void);
uint8_t  VL53L0X_getSpadInfo(uint8_t *count, uint8_t *type_is_aperture);
void     VL53L0X_getSequenceStepEnables(SequenceStepEnables *enables);
void     VL53L0X_getSequenceStepTimeouts(SequenceStepEnables const *enables,
                                          SequenceStepTimeouts *timeouts);
uint8_t  VL53L0X_performSingleRefCalibration(uint8_t vhv_init_byte);
uint16_t VL53L0X_decodeTimeout(uint16_t reg_val);
uint16_t VL53L0X_encodeTimeout(uint16_t timeout_mclks);
uint32_t VL53L0X_timeoutMclksToMicroseconds(uint16_t timeout_period_mclks,
                                              uint8_t vcsel_period_pclks);
uint32_t VL53L0X_timeoutMicrosecondsToMclks(uint32_t timeout_period_us,
                                              uint8_t vcsel_period_pclks);

#endif /* __VL53L0X_H */