#ifndef __CHASSIS_H
#define __CHASSIS_H

#include "main.h"
#include <stdint.h>

void Chassis_Init(void);
void Chassis_ResetPID(void);

void Chassis_Stop(void);

void Chassis_Forward_OpenLoop(int16_t pwm);
void Chassis_Backward_OpenLoop(int16_t pwm);
void Chassis_MoveLeft_OpenLoop(int16_t pwm);
void Chassis_MoveRight_OpenLoop(int16_t pwm);
void Chassis_TurnLeft_OpenLoop(int16_t pwm);
void Chassis_TurnRight_OpenLoop(int16_t pwm);
void Chassis_StartTurnLeft180(void);
uint8_t Chassis_RunTurnLeft180(void);
void Chassis_StartTurnRight180(void);
uint8_t Chassis_RunTurnRight180(void);

void Chassis_StartTurnLeft180(void);
uint8_t Chassis_RunTurnLeft180(void);
void Chassis_StartTurnRight180(void);
uint8_t Chassis_RunTurnRight180(void);

void Chassis_StartStraight(void);
void Chassis_RunStraightPID(int16_t target_speed);
void Chassis_RunStrafeLeftPID(int16_t target_speed);
void Chassis_RunStrafeRightPID(int16_t target_speed);
void Chassis_DebugWheelPID(int16_t s1, int16_t s2, int16_t s3, int16_t s4);

#endif
