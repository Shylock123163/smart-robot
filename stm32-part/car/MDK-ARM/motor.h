#ifndef __MOTOR_H
#define __MOTOR_H

#include "main.h"
#include "tim.h"

#define MOTOR_PWM_MAX  99

void Motor_Init(void);
void Motor_SetPWM(int16_t pwm1, int16_t pwm2, int16_t pwm3, int16_t pwm4);
void Motor_Stop(void);
void Motor_Forward(int16_t speed);
void Motor_Backward(int16_t speed);
void Motor_TurnLeft(int16_t speed);
void Motor_TurnRight(int16_t speed);
void Motor_MoveLeft(int16_t speed);   
void Motor_MoveRight(int16_t speed);  

#endif
