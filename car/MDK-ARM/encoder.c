#include "encoder.h"
#include "main.h"  

#ifndef u16
#define u16 uint16_t
#endif

/* 添加编码器数据结构 */
typedef struct {
    int16_t last_count;     // 上次计数值
    int16_t current_count;  // 当前计数值
    float speed;            // 计算出的速度
    uint32_t last_time;     // 上次读取时间
} Encoder_DataType;

static Encoder_DataType encoder_data[4];  // 0:TIM2, 1:TIM3, 2:TIM4, 3:TIM5

/* TIM2初始化为编码器接口 - PA15(TIM2_CH1), PB3(TIM2_CH2) */
void Encoder_Init_TIM2(uint16_t arr, uint16_t psc)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    
    /* 使能时钟 */
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    
    /* 关键：禁用JTAG但保留SWD，防止PA13/PA14被TIM2重映射占用 */
    __HAL_AFIO_REMAP_SWJ_NOJTAG();
    
    /* 完全重映射TIM2到PA15,PB3 */
    AFIO->MAPR |= AFIO_MAPR_TIM2_REMAP_FULLREMAP;
    
    /* 配置PA15为浮空输入 */
    GPIO_InitStructure.Pin = GPIO_PIN_15;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* 配置PB3为浮空输入 */
    GPIO_InitStructure.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    /* 定时器基础配置 */
    TIM2->ARR = arr;
    TIM2->PSC = psc;
    TIM2->CR1 &= ~TIM_CR1_CKD;
    TIM2->CR1 |= TIM_CLOCKDIVISION_DIV1;
    
    /* 编码器模式3：TI1和TI2同时计数 */
    TIM2->SMCR &= ~TIM_SMCR_SMS;
    TIM2->SMCR |= TIM_ENCODERMODE_TI12;
    
    /* 输入捕获极性：上升沿触发 */
    TIM2->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC2P);
    
    /* 配置输入捕获通道映射 */
    TIM2->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_CC2S);
    TIM2->CCMR1 |= TIM_CCMR1_CC1S_0;  // IC1映射到TI1
    TIM2->CCMR1 |= TIM_CCMR1_CC2S_0;  // IC2映射到TI2
    
    /* 配置输入滤波器 */
    TIM2->CCMR1 |= (10 << 4);   // IC1F = 10
    TIM2->CCMR1 |= (10 << 12);  // IC2F = 10
    
    /* 使能定时器 */
    TIM2->CR1 |= TIM_CR1_CEN;
    
    encoder_data[0].last_count = 0;
    encoder_data[0].last_time = HAL_GetTick();
}

/* TIM3初始化为编码器接口 - PB4(TIM3_CH1), PB5(TIM3_CH2) */
void Encoder_Init_TIM3(uint16_t arr, uint16_t psc)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    
    /* 使能时钟 */
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /* PB4(TIM3_CH1), PB5(TIM3_CH2)配置为浮空输入 */
    GPIO_InitStructure.Pin = GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    /* 定时器基础配置 */
    TIM3->ARR = arr;
    TIM3->PSC = psc;
    TIM3->CR1 &= ~TIM_CR1_CKD;
    TIM3->CR1 |= TIM_CLOCKDIVISION_DIV1;
    
    /* 编码器模式 */
    TIM3->SMCR &= ~TIM_SMCR_SMS;
    TIM3->SMCR |= TIM_ENCODERMODE_TI12;
    
    /* 极性配置 */
    TIM3->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC2P);
    
    /* 滤波器配置 */
    TIM3->CCMR1 |= (10 << 4);
    TIM3->CCMR1 |= (10 << 12);
    
    /* 使能定时器 */
    TIM3->CR1 |= TIM_CR1_CEN;
    
    encoder_data[1].last_count = 0;
    encoder_data[1].last_time = HAL_GetTick();
}

/* TIM4初始化为编码器接口 - PB6(TIM4_CH1), PB7(TIM4_CH2) */
void Encoder_Init_TIM4(uint16_t arr, uint16_t psc)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    
    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    GPIO_InitStructure.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    TIM4->ARR = arr;
    TIM4->PSC = psc;
    TIM4->CR1 &= ~TIM_CR1_CKD;
    TIM4->CR1 |= TIM_CLOCKDIVISION_DIV1;
    
    TIM4->SMCR &= ~TIM_SMCR_SMS;
    TIM4->SMCR |= TIM_ENCODERMODE_TI12;
    
    TIM4->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC2P);
    
    TIM4->CCMR1 |= (10 << 4);
    TIM4->CCMR1 |= (10 << 12);
    
    TIM4->CR1 |= TIM_CR1_CEN;
    
    encoder_data[2].last_count = 0;
    encoder_data[2].last_time = HAL_GetTick();
}

/* TIM5初始化为编码器接口 - PA0(TIM5_CH1), PA1(TIM5_CH2) */
void Encoder_Init_TIM5(uint16_t arr, uint16_t psc)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    
    __HAL_RCC_TIM5_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitStructure.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    TIM5->ARR = arr;
    TIM5->PSC = psc;
    TIM5->CR1 &= ~TIM_CR1_CKD;
    TIM5->CR1 |= TIM_CLOCKDIVISION_DIV1;
    
    TIM5->SMCR &= ~TIM_SMCR_SMS;
    TIM5->SMCR |= TIM_ENCODERMODE_TI12;
    
    TIM5->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC2P);
    
    TIM5->CCMR1 |= (10 << 4);
    TIM5->CCMR1 |= (10 << 12);
    
    TIM5->CR1 |= TIM_CR1_CEN;
    
    encoder_data[3].last_count = 0;
    encoder_data[3].last_time = HAL_GetTick();
}

/* 批量初始化所有编码器 */
void Encoder_Init_All(uint16_t arr, uint16_t psc)
{
    Encoder_Init_TIM2(arr, psc);
    Encoder_Init_TIM3(arr, psc);
    Encoder_Init_TIM4(arr, psc);
    Encoder_Init_TIM5(arr, psc);
}

/* 读取TIM2编码器计数 */
int Read_Encoder_TIM2(void)
{
    int16_t Encoder_TIM;
    Encoder_TIM = (int16_t)TIM2->CNT;
    encoder_data[0].current_count = Encoder_TIM;
    TIM2->CNT = 0;
    return Encoder_TIM;
}

/* 读取TIM3编码器计数 */
int Read_Encoder_TIM3(void)
{
    int16_t Encoder_TIM;
    Encoder_TIM = (int16_t)TIM3->CNT;
    encoder_data[1].current_count = Encoder_TIM;
    TIM3->CNT = 0;
    return Encoder_TIM;
}

/* 读取TIM4编码器计数 */
int Read_Encoder_TIM4(void)
{
    int16_t Encoder_TIM;
    Encoder_TIM = (int16_t)TIM4->CNT;
    encoder_data[2].current_count = Encoder_TIM;
    TIM4->CNT = 0;
    return Encoder_TIM;
}

/* 读取TIM5编码器计数 */
int Read_Encoder_TIM5(void)
{
    int16_t Encoder_TIM;
    Encoder_TIM = (int16_t)TIM5->CNT;
    encoder_data[3].current_count = Encoder_TIM;
    TIM5->CNT = 0;
    return Encoder_TIM;
}

/* 更新所有编码器值（供chassis.c调用）*/
void Encoder_Update(void)
{
    Read_Encoder_TIM2();
    Read_Encoder_TIM3();
    Read_Encoder_TIM4();
    Read_Encoder_TIM5();
}

/* 获取编码器速度（供chassis.c调用）*/
float Encoder_GetSpeed(uint8_t motor_id)
{
    if(motor_id >= 4) return 0;
    
    static uint32_t last_time[4] = {0, 0, 0, 0};  // 修复：每个电机独立计时
    uint32_t current_time = HAL_GetTick();
    float dt = (current_time - last_time[motor_id]) / 1000.0f;
    
    if(dt > 0.001f)
    {
        encoder_data[motor_id].speed = encoder_data[motor_id].current_count / dt;
        last_time[motor_id] = current_time;
    }
    
    return encoder_data[motor_id].speed;
}