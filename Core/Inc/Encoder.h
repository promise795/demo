#ifndef __ENCODER_H
#define __ENCODER_H
#include "stm32f1xx_hal.h"

void Encoder_Init(void);
int32_t Encoder_ReadDelta(void);
int32_t Encoder_GetPosition(void);

#endif
