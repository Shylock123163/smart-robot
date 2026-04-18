#include "vl53l0x.h"
#include <string.h>

/* ══════════════════════════════════════
   内部全局变量
   ══════════════════════════════════════ */
static uint8_t  VL53L0X_address = VL53L0X_DEFAULT_ADDR;
static uint8_t  stop_variable   = 0;
static uint32_t measurement_timing_budget_us = 0;
static uint8_t  did_timeout     = 0;
static uint16_t io_timeout      = 500;
static uint16_t timeout_start_ms = 0;

/* 宏定义 */
#define TRUE  1
#define FALSE 0
#define decodeVcselPeriod(reg_val)      (((reg_val) + 1) << 1)
#define encodeVcselPeriod(period_pclks) (((period_pclks) >> 1) - 1)
#define calcMacroPeriod(vcsel_period_pclks) \
    ((((uint32_t)2304 * (vcsel_period_pclks) * 1655) + 500) / 1000)
#define startTimeout()       (timeout_start_ms = 0)
#define checkTimeoutExpired() \
    ((io_timeout > 0 && timeout_start_ms > io_timeout) ? 1 : 0)

void VL53L0X_SetCurrentAddr(uint8_t addr)
{
    VL53L0X_address = addr;
}

void VL53L0X_writeReg(uint8_t reg, uint8_t value)
{
    HAL_I2C_Mem_Write(&VL53L0X_I2C,
                      VL53L0X_address,
                      reg, I2C_MEMADD_SIZE_8BIT,
                      &value, 1,
                      100);
}

void VL53L0X_writeReg16Bit(uint8_t reg, uint16_t value)
{
    uint8_t buf[2];
    buf[0] = (uint8_t)((value >> 8) & 0xFF);
    buf[1] = (uint8_t)(value & 0xFF);
    HAL_I2C_Mem_Write(&VL53L0X_I2C,
                      VL53L0X_address,
                      reg, I2C_MEMADD_SIZE_8BIT,
                      buf, 2,
                      100);
}

void VL53L0X_writeReg32Bit(uint8_t reg, uint32_t value)
{
    uint8_t buf[4];
    buf[0] = (uint8_t)((value >> 24) & 0xFF);
    buf[1] = (uint8_t)((value >> 16) & 0xFF);
    buf[2] = (uint8_t)((value >> 8)  & 0xFF);
    buf[3] = (uint8_t)(value & 0xFF);
    HAL_I2C_Mem_Write(&VL53L0X_I2C,
                      VL53L0X_address,
                      reg, I2C_MEMADD_SIZE_8BIT,
                      buf, 4,
                      100);
}

uint8_t VL53L0X_readReg(uint8_t reg)
{
    uint8_t value = 0;
    HAL_I2C_Mem_Read(&VL53L0X_I2C,
                     VL53L0X_address,
                     reg, I2C_MEMADD_SIZE_8BIT,
                     &value, 1,
                     100);
    return value;
}

uint16_t VL53L0X_readReg16Bit(uint8_t reg)
{
    uint8_t buf[2] = {0};
    HAL_I2C_Mem_Read(&VL53L0X_I2C,
                     VL53L0X_address,
                     reg, I2C_MEMADD_SIZE_8BIT,
                     buf, 2,
                     100);
    return (uint16_t)((buf[0] << 8) | buf[1]);
}

void VL53L0X_writeMulti(uint8_t reg, uint8_t const *src, uint8_t count)
{
    HAL_I2C_Mem_Write(&VL53L0X_I2C,
                      VL53L0X_address,
                      reg, I2C_MEMADD_SIZE_8BIT,
                      (uint8_t *)src, count,
                      100);
}

void VL53L0X_readMulti(uint8_t reg, uint8_t *dst, uint8_t count)
{
    HAL_I2C_Mem_Read(&VL53L0X_I2C,
                     VL53L0X_address,
                     reg, I2C_MEMADD_SIZE_8BIT,
                     dst, count,
                     100);
}

void VL53L0X_setAddress(uint8_t new_addr)
{
    VL53L0X_writeReg(I2C_SLAVE_DEVICE_ADDRESS, new_addr >> 1);
    VL53L0X_address = new_addr;
    HAL_Delay(10);
}

uint8_t VL53L0X_init(uint8_t io_2v8)
{
    uint8_t spad_count;
    uint8_t spad_type_is_aperture;
    uint8_t ref_spad_map[6];
    uint8_t spads_enabled = 0;
    uint8_t i;

    io_timeout  = 500;
    did_timeout = 0;

    if(io_2v8)
    {
        VL53L0X_writeReg(VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV,
                         VL53L0X_readReg(VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV) | 0x01);
    }

    VL53L0X_writeReg(0x88, 0x00);
    VL53L0X_writeReg(0x80, 0x01);
    VL53L0X_writeReg(0xFF, 0x01);
    VL53L0X_writeReg(0x00, 0x00);
    stop_variable = VL53L0X_readReg(0x91);
    VL53L0X_writeReg(0x00, 0x01);
    VL53L0X_writeReg(0xFF, 0x00);
    VL53L0X_writeReg(0x80, 0x00);

    VL53L0X_writeReg(MSRC_CONFIG_CONTROL,
                     VL53L0X_readReg(MSRC_CONFIG_CONTROL) | 0x12);

    VL53L0X_setSignalRateLimit(0.25);
    VL53L0X_writeReg(SYSTEM_SEQUENCE_CONFIG, 0xFF);

    if(!VL53L0X_getSpadInfo(&spad_count, &spad_type_is_aperture))
        return FALSE;

    VL53L0X_readMulti(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);

    VL53L0X_writeReg(0xFF, 0x01);
    VL53L0X_writeReg(DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
    VL53L0X_writeReg(DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C);
    VL53L0X_writeReg(0xFF, 0x00);
    VL53L0X_writeReg(GLOBAL_CONFIG_REF_EN_START_SELECT, 0xB4);

    uint8_t first_spad_to_enable = spad_type_is_aperture ? 12 : 0;

    for(i = 0; i < 48; i++)
    {
        if(i < first_spad_to_enable || spads_enabled == spad_count)
        {
            ref_spad_map[i / 8] &= ~(1 << (i % 8));
        }
        else if((ref_spad_map[i / 8] >> (i % 8)) & 0x1)
        {
            spads_enabled++;
        }
    }
    VL53L0X_writeMulti(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);

    VL53L0X_writeReg(0xFF, 0x01); VL53L0X_writeReg(0x00, 0x00);
    VL53L0X_writeReg(0xFF, 0x00); VL53L0X_writeReg(0x09, 0x00);
    VL53L0X_writeReg(0x10, 0x00); VL53L0X_writeReg(0x11, 0x00);
    VL53L0X_writeReg(0x24, 0x01); VL53L0X_writeReg(0x25, 0xFF);
    VL53L0X_writeReg(0x75, 0x00); VL53L0X_writeReg(0xFF, 0x01);
    VL53L0X_writeReg(0x4E, 0x2C); VL53L0X_writeReg(0x48, 0x00);
    VL53L0X_writeReg(0x30, 0x20); VL53L0X_writeReg(0xFF, 0x00);
    VL53L0X_writeReg(0x30, 0x09); VL53L0X_writeReg(0x54, 0x00);
    VL53L0X_writeReg(0x31, 0x04); VL53L0X_writeReg(0x32, 0x03);
    VL53L0X_writeReg(0x40, 0x83); VL53L0X_writeReg(0x46, 0x25);
    VL53L0X_writeReg(0x60, 0x00); VL53L0X_writeReg(0x27, 0x00);
    VL53L0X_writeReg(0x50, 0x06); VL53L0X_writeReg(0x51, 0x00);
    VL53L0X_writeReg(0x52, 0x96); VL53L0X_writeReg(0x56, 0x08);
    VL53L0X_writeReg(0x57, 0x30); VL53L0X_writeReg(0x61, 0x00);
    VL53L0X_writeReg(0x62, 0x00); VL53L0X_writeReg(0x64, 0x00);
    VL53L0X_writeReg(0x65, 0x00); VL53L0X_writeReg(0x66, 0xA0);
    VL53L0X_writeReg(0xFF, 0x01); VL53L0X_writeReg(0x22, 0x32);
    VL53L0X_writeReg(0x47, 0x14); VL53L0X_writeReg(0x49, 0xFF);
    VL53L0X_writeReg(0x4A, 0x00); VL53L0X_writeReg(0xFF, 0x00);
    VL53L0X_writeReg(0x7A, 0x0A); VL53L0X_writeReg(0x7B, 0x00);
    VL53L0X_writeReg(0x78, 0x21); VL53L0X_writeReg(0xFF, 0x01);
    VL53L0X_writeReg(0x23, 0x34); VL53L0X_writeReg(0x42, 0x00);
    VL53L0X_writeReg(0x44, 0xFF); VL53L0X_writeReg(0x45, 0x26);
    VL53L0X_writeReg(0x46, 0x05); VL53L0X_writeReg(0x40, 0x40);
    VL53L0X_writeReg(0x0E, 0x06); VL53L0X_writeReg(0x20, 0x1A);
    VL53L0X_writeReg(0x43, 0x40); VL53L0X_writeReg(0xFF, 0x00);
    VL53L0X_writeReg(0x34, 0x03); VL53L0X_writeReg(0x35, 0x44);
    VL53L0X_writeReg(0xFF, 0x01); VL53L0X_writeReg(0x31, 0x04);
    VL53L0X_writeReg(0x4B, 0x09); VL53L0X_writeReg(0x4C, 0x05);
    VL53L0X_writeReg(0x4D, 0x04); VL53L0X_writeReg(0xFF, 0x00);
    VL53L0X_writeReg(0x44, 0x00); VL53L0X_writeReg(0x45, 0x20);
    VL53L0X_writeReg(0x47, 0x08); VL53L0X_writeReg(0x48, 0x28);
    VL53L0X_writeReg(0x67, 0x00); VL53L0X_writeReg(0x70, 0x04);
    VL53L0X_writeReg(0x71, 0x01); VL53L0X_writeReg(0x72, 0xFE);
    VL53L0X_writeReg(0x76, 0x00); VL53L0X_writeReg(0x77, 0x00);
    VL53L0X_writeReg(0xFF, 0x01); VL53L0X_writeReg(0x0D, 0x01);
    VL53L0X_writeReg(0xFF, 0x00); VL53L0X_writeReg(0x80, 0x01);
    VL53L0X_writeReg(0x01, 0xF8); VL53L0X_writeReg(0xFF, 0x01);
    VL53L0X_writeReg(0x8E, 0x01); VL53L0X_writeReg(0x00, 0x01);
    VL53L0X_writeReg(0xFF, 0x00); VL53L0X_writeReg(0x80, 0x00);

    VL53L0X_writeReg(SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
    VL53L0X_writeReg(GPIO_HV_MUX_ACTIVE_HIGH,
                     VL53L0X_readReg(GPIO_HV_MUX_ACTIVE_HIGH) & ~0x10);
    VL53L0X_writeReg(SYSTEM_INTERRUPT_CLEAR, 0x01);

    measurement_timing_budget_us = VL53L0X_getMeasurementTimingBudget();

    VL53L0X_writeReg(SYSTEM_SEQUENCE_CONFIG, 0xE8);
    VL53L0X_setMeasurementTimingBudget(measurement_timing_budget_us);

    VL53L0X_writeReg(SYSTEM_SEQUENCE_CONFIG, 0x01);
    if(!VL53L0X_performSingleRefCalibration(0x40)) return FALSE;

    VL53L0X_writeReg(SYSTEM_SEQUENCE_CONFIG, 0x02);
    if(!VL53L0X_performSingleRefCalibration(0x00)) return FALSE;

    VL53L0X_writeReg(SYSTEM_SEQUENCE_CONFIG, 0xE8);

    return TRUE;
}

uint8_t VL53L0X_Init_All(void)
{
    HAL_GPIO_WritePin(XSHUT_L_PORT, XSHUT_L_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(XSHUT_F_PORT, XSHUT_F_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(XSHUT_R_PORT, XSHUT_R_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);

    HAL_GPIO_WritePin(XSHUT_L_PORT, XSHUT_L_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
    VL53L0X_SetCurrentAddr(VL53L0X_DEFAULT_ADDR);
    if(!VL53L0X_init(1)) return 11;
    VL53L0X_setAddress(VL53L0X_ADDR_LEFT);
    VL53L0X_startContinuous(0);

    HAL_GPIO_WritePin(XSHUT_F_PORT, XSHUT_F_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
    VL53L0X_SetCurrentAddr(VL53L0X_DEFAULT_ADDR);
    if(!VL53L0X_init(1)) return 22;
    VL53L0X_setAddress(VL53L0X_ADDR_FRONT);
    VL53L0X_startContinuous(0);

    HAL_GPIO_WritePin(XSHUT_R_PORT, XSHUT_R_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
    VL53L0X_SetCurrentAddr(VL53L0X_DEFAULT_ADDR);
    if(!VL53L0X_init(1)) return 33;
    VL53L0X_setAddress(VL53L0X_ADDR_RIGHT);
    VL53L0X_startContinuous(0);

    return 0;
}

uint16_t VL53L0X_ReadDistance(uint8_t sensor_id)
{
    switch(sensor_id)
    {
        case 0: VL53L0X_SetCurrentAddr(VL53L0X_ADDR_LEFT);  break;
        case 1: VL53L0X_SetCurrentAddr(VL53L0X_ADDR_FRONT); break;
        case 2: VL53L0X_SetCurrentAddr(VL53L0X_ADDR_RIGHT); break;
        default: return 9999;
    }
    return VL53L0X_readRangeContinuousMillimeters();
}

void VL53L0X_startContinuous(uint32_t period_ms)
{
    VL53L0X_writeReg(0x80, 0x01);
    VL53L0X_writeReg(0xFF, 0x01);
    VL53L0X_writeReg(0x00, 0x00);
    VL53L0X_writeReg(0x91, stop_variable);
    VL53L0X_writeReg(0x00, 0x01);
    VL53L0X_writeReg(0xFF, 0x00);
    VL53L0X_writeReg(0x80, 0x00);

    if(period_ms != 0)
    {
        uint16_t osc_calibrate_val = VL53L0X_readReg16Bit(OSC_CALIBRATE_VAL);
        if(osc_calibrate_val != 0)
            period_ms *= osc_calibrate_val;
        VL53L0X_writeReg32Bit(SYSTEM_INTERMEASUREMENT_PERIOD, period_ms);
        VL53L0X_writeReg(SYSRANGE_START, 0x04);
    }
    else
    {
        VL53L0X_writeReg(SYSRANGE_START, 0x02);
    }
}

uint16_t VL53L0X_readRangeContinuousMillimeters(void)
{
    uint16_t range;
    uint32_t timeout_cnt = 0;

    while((VL53L0X_readReg(RESULT_INTERRUPT_STATUS) & 0x07) == 0)
    {
        timeout_cnt++;
        if(timeout_cnt > 10000)
        {
            did_timeout = TRUE;
            return 9999;
        }
    }

    range = VL53L0X_readReg16Bit(RESULT_RANGE_STATUS + 10);
    VL53L0X_writeReg(SYSTEM_INTERRUPT_CLEAR, 0x01);

    return range;
}

uint8_t VL53L0X_setSignalRateLimit(float limit_Mcps)
{
    if(limit_Mcps < 0.0 || limit_Mcps > 511.99) return FALSE;
    VL53L0X_writeReg16Bit(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT,
                           (uint16_t)(limit_Mcps * (1 << 7)));
    return TRUE;
}

uint8_t VL53L0X_setMeasurementTimingBudget(uint32_t budget_us)
{
    SequenceStepEnables  enables;
    SequenceStepTimeouts timeouts;

    uint16_t const StartOverhead      = 1320;
    uint16_t const EndOverhead        = 960;
    uint16_t const MsrcOverhead       = 660;
    uint16_t const TccOverhead        = 590;
    uint16_t const DssOverhead        = 690;
    uint16_t const PreRangeOverhead   = 660;
    uint16_t const FinalRangeOverhead = 550;
    uint32_t const MinTimingBudget    = 20000;

    if(budget_us < MinTimingBudget) return FALSE;

    uint32_t used_budget_us = StartOverhead + EndOverhead;
    VL53L0X_getSequenceStepEnables(&enables);
    VL53L0X_getSequenceStepTimeouts(&enables, &timeouts);

    if(enables.tcc)
        used_budget_us += (timeouts.msrc_dss_tcc_us + TccOverhead);
    if(enables.dss)
        used_budget_us += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
    else if(enables.msrc)
        used_budget_us += (timeouts.msrc_dss_tcc_us + MsrcOverhead);
    if(enables.pre_range)
        used_budget_us += (timeouts.pre_range_us + PreRangeOverhead);
    if(enables.final_range)
    {
        used_budget_us += FinalRangeOverhead;
        if(used_budget_us > budget_us) return FALSE;

        uint32_t final_range_timeout_us   = budget_us - used_budget_us;
        uint16_t final_range_timeout_mclks =
            (uint16_t)VL53L0X_timeoutMicrosecondsToMclks(
                final_range_timeout_us,
                timeouts.final_range_vcsel_period_pclks);
        if(enables.pre_range)
            final_range_timeout_mclks += timeouts.pre_range_mclks;

        VL53L0X_writeReg16Bit(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI,
                               VL53L0X_encodeTimeout(final_range_timeout_mclks));
        measurement_timing_budget_us = budget_us;
    }
    return TRUE;
}

uint32_t VL53L0X_getMeasurementTimingBudget(void)
{
    SequenceStepEnables  enables;
    SequenceStepTimeouts timeouts;

    uint16_t const StartOverhead      = 1910;
    uint16_t const EndOverhead        = 960;
    uint16_t const MsrcOverhead       = 660;
    uint16_t const TccOverhead        = 590;
    uint16_t const DssOverhead        = 690;
    uint16_t const PreRangeOverhead   = 660;
    uint16_t const FinalRangeOverhead = 550;

    uint32_t budget_us = StartOverhead + EndOverhead;
    VL53L0X_getSequenceStepEnables(&enables);
    VL53L0X_getSequenceStepTimeouts(&enables, &timeouts);

    if(enables.tcc)
        budget_us += (timeouts.msrc_dss_tcc_us + TccOverhead);
    if(enables.dss)
        budget_us += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
    else if(enables.msrc)
        budget_us += (timeouts.msrc_dss_tcc_us + MsrcOverhead);
    if(enables.pre_range)
        budget_us += (timeouts.pre_range_us + PreRangeOverhead);
    if(enables.final_range)
        budget_us += (timeouts.final_range_us + FinalRangeOverhead);

    measurement_timing_budget_us = budget_us;
    return budget_us;
}

uint8_t VL53L0X_getSpadInfo(uint8_t *count, uint8_t *type_is_aperture)
{
    uint8_t tmp;
    uint32_t timeout_cnt = 0;

    VL53L0X_writeReg(0x80, 0x01);
    VL53L0X_writeReg(0xFF, 0x01);
    VL53L0X_writeReg(0x00, 0x00);
    VL53L0X_writeReg(0xFF, 0x06);
    VL53L0X_writeReg(0x83, VL53L0X_readReg(0x83) | 0x04);
    VL53L0X_writeReg(0xFF, 0x07);
    VL53L0X_writeReg(0x81, 0x01);
    VL53L0X_writeReg(0x80, 0x01);
    VL53L0X_writeReg(0x94, 0x6b);
    VL53L0X_writeReg(0x83, 0x00);

    while(VL53L0X_readReg(0x83) == 0x00)
    {
        timeout_cnt++;
        if(timeout_cnt > 10000) return FALSE;
    }

    VL53L0X_writeReg(0x83, 0x01);
    tmp = VL53L0X_readReg(0x92);
    *count            = tmp & 0x7F;
    *type_is_aperture = (tmp >> 7) & 0x01;

    VL53L0X_writeReg(0x81, 0x00);
    VL53L0X_writeReg(0xFF, 0x06);
    VL53L0X_writeReg(0x83, VL53L0X_readReg(0x83) & ~0x04);
    VL53L0X_writeReg(0xFF, 0x01);
    VL53L0X_writeReg(0x00, 0x01);
    VL53L0X_writeReg(0xFF, 0x00);
    VL53L0X_writeReg(0x80, 0x00);

    return TRUE;
}

void VL53L0X_getSequenceStepEnables(SequenceStepEnables *enables)
{
    uint8_t sequence_config = VL53L0X_readReg(SYSTEM_SEQUENCE_CONFIG);
    enables->tcc         = (sequence_config >> 4) & 0x1;
    enables->dss         = (sequence_config >> 3) & 0x1;
    enables->msrc        = (sequence_config >> 2) & 0x1;
    enables->pre_range   = (sequence_config >> 6) & 0x1;
    enables->final_range = (sequence_config >> 7) & 0x1;
}

void VL53L0X_getSequenceStepTimeouts(SequenceStepEnables const *enables,
                                      SequenceStepTimeouts *timeouts)
{
    timeouts->pre_range_vcsel_period_pclks =
        decodeVcselPeriod(VL53L0X_readReg(PRE_RANGE_CONFIG_VCSEL_PERIOD));

    timeouts->msrc_dss_tcc_mclks =
        VL53L0X_readReg(MSRC_CONFIG_TIMEOUT_MACROP) + 1;
    timeouts->msrc_dss_tcc_us =
        VL53L0X_timeoutMclksToMicroseconds(timeouts->msrc_dss_tcc_mclks,
                               timeouts->pre_range_vcsel_period_pclks);

    timeouts->pre_range_mclks =
        VL53L0X_decodeTimeout(VL53L0X_readReg16Bit(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
    timeouts->pre_range_us =
        VL53L0X_timeoutMclksToMicroseconds(timeouts->pre_range_mclks,
                               timeouts->pre_range_vcsel_period_pclks);

    timeouts->final_range_vcsel_period_pclks =
        decodeVcselPeriod(VL53L0X_readReg(FINAL_RANGE_CONFIG_VCSEL_PERIOD));

    timeouts->final_range_mclks =
        VL53L0X_decodeTimeout(VL53L0X_readReg16Bit(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI));

    if(enables->pre_range)
        timeouts->final_range_mclks -= timeouts->pre_range_mclks;

    timeouts->final_range_us =
        VL53L0X_timeoutMclksToMicroseconds(timeouts->final_range_mclks,
                               timeouts->final_range_vcsel_period_pclks);
}

uint8_t VL53L0X_performSingleRefCalibration(uint8_t vhv_init_byte)
{
    uint32_t timeout_cnt = 0;
    VL53L0X_writeReg(SYSRANGE_START, 0x01 | vhv_init_byte);

    while((VL53L0X_readReg(RESULT_INTERRUPT_STATUS) & 0x07) == 0)
    {
        timeout_cnt++;
        if(timeout_cnt > 10000) return FALSE;
    }

    VL53L0X_writeReg(SYSTEM_INTERRUPT_CLEAR, 0x01);
    VL53L0X_writeReg(SYSRANGE_START, 0x00);
    return TRUE;
}

/* ══════════════════════════════════════
   修复点：拆分移位运算，兼容Keil V5编译器
   原代码在强制转换内嵌套移位，Keil V5不支持
   ══════════════════════════════════════ */
uint16_t VL53L0X_decodeTimeout(uint16_t reg_val)
{
    uint16_t msb = (reg_val & 0xFF00) >> 8;   // 高字节作为移位量
    uint16_t lsb = (reg_val & 0x00FF);         // 低字节作为基数
    return (uint16_t)(lsb << msb) + 1;
}

uint16_t VL53L0X_encodeTimeout(uint16_t timeout_mclks)
{
    uint32_t ls_byte = 0;
    uint16_t ms_byte = 0;

    if(timeout_mclks > 0)
    {
        ls_byte = timeout_mclks - 1;
        while((ls_byte & 0xFFFFFF00) > 0)
        {
            ls_byte >>= 1;
            ms_byte++;
        }
        return (ms_byte << 8) | (ls_byte & 0xFF);
    }
    return 0;
}

uint32_t VL53L0X_timeoutMclksToMicroseconds(uint16_t timeout_period_mclks,
                                              uint8_t vcsel_period_pclks)
{
    uint32_t macro_period_ns = calcMacroPeriod(vcsel_period_pclks);
    return ((timeout_period_mclks * macro_period_ns) +
            (macro_period_ns / 2)) / 1000;
}

uint32_t VL53L0X_timeoutMicrosecondsToMclks(uint32_t timeout_period_us,
                                              uint8_t vcsel_period_pclks)
{
    uint32_t macro_period_ns = calcMacroPeriod(vcsel_period_pclks);
    return (((timeout_period_us * 1000) +
             (macro_period_ns / 2)) / macro_period_ns);
}