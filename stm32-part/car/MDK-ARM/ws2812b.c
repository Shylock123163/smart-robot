/**
  ******************************************************************************
  * @file       ws2812.c
  * @author     Adapted from embedfire WS2813 example
  * @version    V1.0
  * @date       2026
  * @brief      WS2812B RGB灯带驱动（PWM+DMA方式，TIM8通道3）
  * 
  * @note       硬件配置：
  *             - MCU: STM32F103ZET6
  *             - 定时器: TIM8, 通道3 (PWM模式)
  *             - 引脚: 根据TIM8_CH3的复用功能（通常是PC8）
  *             - 时钟: 72MHz, PWM周期 1.25us (800KHz)
  ******************************************************************************
  */

#include "ws2812b.h"
#include "tim.h"          // 包含TIM8初始化函数 MX_TIM8_Init()
#include <string.h>

/* 调试输出宏（如无串口可注释掉） */
// #define WS2812_DEBUG

#ifdef WS2812_DEBUG
    #include "usart.h"
    #define DBG_PRINTF printf
#else
    #define DBG_PRINTF(...)
#endif

/*==============================================================================
 * 配置参数（根据实际情况修改）
 *============================================================================*/

/** 
  * @brief LED数量
  */
#ifndef LED_NUM
    #define LED_NUM    30
#endif

/** 
  * @brief 单个LED的数据位数（24位：G+R+B）
  */
#define BITS_PER_LED    24

/** 
  * @brief 复位信号长度（单位：PWM周期）
  * @note  WS2812B复位信号要求 >50us，每个PWM周期1.25us，
  *        所以至少需要 50/1.25 = 40个周期。这里取80个确保可靠。
  */
#define RESET_CYCLES    80

/** 
  * @brief DMA缓冲区总长度
  */
#define DMA_BUF_SIZE    (RESET_CYCLES + LED_NUM * BITS_PER_LED)

/** 
  * @brief 逻辑1和逻辑0对应的PWM比较值
  * @note  在72MHz、PSC=0、ARR=89时，每个计数步长约13.9ns
  *        逻辑1高电平800ns -> 800/13.9 ≈ 58
  *        逻辑0高电平400ns -> 400/13.9 ≈ 29
  *        以下数值可能需要用逻辑分析仪微调
  */
#define TIMING_1        60      // 逻辑1的比较值
#define TIMING_0        30      // 逻辑0的比较值

/*==============================================================================
 * 全局变量
 *============================================================================*/

/** 
  * @brief DMA发送缓冲区
  * @note  使用uint16_t数组，对应TIM比较寄存器的宽度
  */
static uint16_t DMABuffer[DMA_BUF_SIZE] = {0};

/** 
  * @brief LED颜色缓存（方便单独控制每个灯）
  * @note  存储顺序为 [LED索引][G, R, B]
  */
static uint8_t led_cache[LED_NUM][3] = {0};

/*==============================================================================
 * 静态函数声明
 *============================================================================*/

/**
  * @brief 更新DMA缓冲区
  * @note  将led_cache中的颜色数据转换为PWM比较值填充到DMABuffer
  */
static void WS2812_UpdateDMABuffer(void);

/*==============================================================================
 * 函数实现
 *============================================================================*/

/**
  * @brief  WS2812初始化
  * @param  无
  * @retval 无
  */
void WS2812_Init(void)
{
    // 1. 初始化TIM8（已在CubeMX中配置，这里调用生成好的函数）
    MX_TIM8_Init();
    
	// 关键！！！
    __HAL_TIM_MOE_ENABLE(&htim8);

    // 2. 清除LED缓存
    memset(led_cache, 0, sizeof(led_cache));
    
    // 3. 清空DMA缓冲区
    memset(DMABuffer, 0, sizeof(DMABuffer));
    
    // 4. 发送一次复位信号，确保灯带初始状态为灭
    WS2812_Send();
    
    DBG_PRINTF("WS2812 initialized, LED count: %d\r\n", LED_NUM);
}

/**
  * @brief  更新DMA缓冲区
  * @note   将缓存中的颜色值按WS2812协议转换为PWM占空比
  * @retval 无
  */
static void WS2812_UpdateDMABuffer(void)
{
    uint16_t i, j;
    uint16_t offset = RESET_CYCLES;  // 跳过复位信号区域
    
    DBG_PRINTF("Updating DMA buffer, offset start: %d\r\n", offset);
    
    // 遍历每个LED
    for(i = 0; i < LED_NUM; i++)
    {
        // 处理绿色通道（G）- WS2812协议先发G
        for(j = 0; j < 8; j++)
        {
            DMABuffer[offset + j] = (led_cache[i][1] & (0x80 >> j)) ? TIMING_1 : TIMING_0;
        }
        
        // 处理红色通道（R）
        for(j = 0; j < 8; j++)
        {
            DMABuffer[offset + j + 8] = (led_cache[i][0] & (0x80 >> j)) ? TIMING_1 : TIMING_0;
        }
        
        // 处理蓝色通道（B）
        for(j = 0; j < 8; j++)
        {
            DMABuffer[offset + j + 16] = (led_cache[i][2] & (0x80 >> j)) ? TIMING_1 : TIMING_0;
        }
        
        offset += BITS_PER_LED;  // 移动到下一个LED的数据区
    }
    
    // 复位信号区（前RESET_CYCLES个数据）保持为0，已经由memset初始化
}

/**
  * @brief  发送LED数据到灯带
  * @note   启动DMA传输，通过TIM8通道3的PWM输出控制WS2812
  * @retval 无
  */

void WS2812_Send(void)
{
    HAL_StatusTypeDef status;
    
    // 1. 根据当前缓存更新DMA缓冲区
    WS2812_UpdateDMABuffer();
    
    // 2. 停止之前的DMA传输（如果有）
    HAL_TIM_PWM_Stop_DMA(&htim8, TIM_CHANNEL_3);
    
    // 3. 启动新的DMA传输
    //    注意：DMA配置为外设到内存？不，这里是内存到外设。
    //    使用HAL_TIM_PWM_Start_DMA，它会自动配置DMA传输方向为内存到外设
    status = HAL_TIM_PWM_Start_DMA(&htim8, TIM_CHANNEL_3,
                                   (uint32_t*)DMABuffer, DMA_BUF_SIZE);
    
    if(status != HAL_OK)
    {
        DBG_PRINTF("DMA start failed: %d\r\n", status);
        // 启动失败处理
        switch(status)
        {
            case HAL_BUSY:
                DBG_PRINTF("DMA busy\r\n");
                break;
            case HAL_ERROR:
                DBG_PRINTF("DMA error\r\n");
                break;
            default:
                break;
        }
    }
    else
    {
        DBG_PRINTF("DMA started, buffer size: %d\r\n", DMA_BUF_SIZE);
    }
}

/**
  * @brief  PWM+DMA传输完成回调
  * @param  htim 触发回调的定时器句柄
  * @retval 无
  */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM8)
    {
        // 停止PWM输出，节省功耗
        HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_3);
        
        // 将比较寄存器设为0，确保引脚为低电平
        __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_3, 0);
        
        DBG_PRINTF("DMA transfer completed\r\n");
    }
}

/**
  * @brief  设置所有LED为同一颜色
  * @param  r 红色分量 (0-255)
  * @param  g 绿色分量 (0-255)
  * @param  b 蓝色分量 (0-255)
  * @retval 无
  */
void WS2812_SetAll(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t i;
    
    DBG_PRINTF("Set all LEDs: R=%d, G=%d, B=%d\r\n", r, g, b);
    
    for(i = 0; i < LED_NUM; i++)
    {
        led_cache[i][0] = r;  // 红色
        led_cache[i][1] = g;  // 绿色
        led_cache[i][2] = b;  // 蓝色
    }
    
    // 发送数据
    WS2812_Send();
}

/**
  * @brief  设置单个LED的颜色
  * @param  index LED索引 (0 ~ LED_NUM-1)
  * @param  r 红色分量 (0-255)
  * @param  g 绿色分量 (0-255)
  * @param  b 蓝色分量 (0-255)
  * @retval 无
  */
void WS2812_SetLED(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if(index >= LED_NUM)
    {
        DBG_PRINTF("LED index out of range: %d\r\n", index);
        return;
    }
    
    led_cache[index][0] = r;
    led_cache[index][1] = g;
    led_cache[index][2] = b;
    
    // 只更新单个LED，但需要重发所有数据
    WS2812_Send();
}

/**
  * @brief  清空所有LED（全灭）
  * @retval 无
  */
void WS2812_Clear(void)
{
    WS2812_SetAll(0, 0, 0);
}

/**
  * @brief  测试函数 - 流水灯效果
  * @param  delay_ms 每步延时（毫秒）
  * @retval 无
  */
void WS2812_Test_Waterfall(uint16_t delay_ms)
{
    uint16_t i;
    
    for(i = 0; i < LED_NUM; i++)
    {
        WS2812_Clear();
        WS2812_SetLED(i, 255, 0, 0);  // 红色
        HAL_Delay(delay_ms);
    }
    
    for(i = 0; i < LED_NUM; i++)
    {
        WS2812_Clear();
        WS2812_SetLED(i, 0, 255, 0);  // 绿色
        HAL_Delay(delay_ms);
    }
    
    for(i = 0; i < LED_NUM; i++)
    {
        WS2812_Clear();
        WS2812_SetLED(i, 0, 0, 255);  // 蓝色
        HAL_Delay(delay_ms);
    }
}

/**
  * @brief  测试函数 - 彩虹呼吸效果
  * @param  cycles 呼吸次数
  * @retval 无
  */
void WS2812_Test_Breathe(uint8_t cycles)
{
    uint16_t i, j;
    uint8_t r, g, b;
    
    for(i = 0; i < cycles; i++)
    {
        // 红色呼吸
        for(j = 0; j < 256; j += 5)
        {
            WS2812_SetAll(j, 0, 0);
            HAL_Delay(10);
        }
        for(j = 255; j > 0; j -= 5)
        {
            WS2812_SetAll(j, 0, 0);
            HAL_Delay(10);
        }
        
        // 绿色呼吸
        for(j = 0; j < 256; j += 5)
        {
            WS2812_SetAll(0, j, 0);
            HAL_Delay(10);
        }
        for(j = 255; j > 0; j -= 5)
        {
            WS2812_SetAll(0, j, 0);
            HAL_Delay(10);
        }
        
        // 蓝色呼吸
        for(j = 0; j < 256; j += 5)
        {
            WS2812_SetAll(0, 0, j);
            HAL_Delay(10);
        }
        for(j = 255; j > 0; j -= 5)
        {
            WS2812_SetAll(0, 0, j);
            HAL_Delay(10);
        }
    }
}
 void TIM8_SwitchToWs2812(void)
  {
      HAL_TIM_PWM_Stop(&htim8,
  TIM_CHANNEL_4);
      HAL_TIM_Base_DeInit(&htim8);

      MX_TIM8_Init();                  //这里是你原来WS2812用的TIM8初始化
      __HAL_TIM_MOE_ENABLE(&htim8);
  }
/*============================= END OF FILE ==================================*/