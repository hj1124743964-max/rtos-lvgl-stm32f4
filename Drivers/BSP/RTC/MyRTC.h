#ifndef MYRTC_H
#define MYRTC_H

#include "stm32f4xx_hal.h"

extern RTC_HandleTypeDef hrtc;
extern uint16_t MyRTC_Time[6];

void MyRTC_Init(void);
void MyRTC_SetTime(void);
void MyRTC_ReadTime(void);


#endif
