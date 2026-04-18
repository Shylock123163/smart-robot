#ifndef __ENCODER_H
#define __ENCODER_H

#include "main.h"
#include "stm32f1xx_hal.h"  

/* 编码器初始化函数 */
void Encoder_Init_TIM2(uint16_t arr, uint16_t psc);  
void Encoder_Init_TIM3(uint16_t arr, uint16_t psc);
void Encoder_Init_TIM4(uint16_t arr, uint16_t psc);
void Encoder_Init_TIM5(uint16_t arr, uint16_t psc);

/* 读取编码器计数值 */
int Read_Encoder_TIM2(void);
int Read_Encoder_TIM3(void);
int Read_Encoder_TIM4(void);
int Read_Encoder_TIM5(void);


/* 批量初始化所有编码器 */
void Encoder_Init_All(uint16_t arr, uint16_t psc);

/* 更新所有编码器值（可选）*/
void Encoder_Update(void);

/* 获取编码器速度（可选）*/
float Encoder_GetSpeed(uint8_t motor_id);

#endif


//#ifndef __ENCODER_H
//#define __ENCODER_H

//#include "stm32f1xx_hal.h"  

//void Encoder_Update(void);

//float Encoder_GetSpeed(uint8_t id);

//#endif