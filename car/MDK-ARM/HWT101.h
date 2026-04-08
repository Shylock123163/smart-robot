#ifndef __HWT101_H
#define __HWT101_H

#include <stdint.h>

extern float fAcc[3];
extern float fGyro[3];
extern float fAngle[3];

int32_t IICreadBytes(uint8_t dev, uint8_t reg, uint8_t *data, uint32_t length);
int32_t IICwriteBytes(uint8_t dev, uint8_t reg, uint8_t *data, uint32_t length);
void HWT101_Init(void);
void HWT101_GetValue(void);

#endif