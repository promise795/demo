#include "Encoder.h"

extern TIM_HandleTypeDef htim2;

static int32_t s_position=0;//累计位置
static int16_t s_last=0;//上一次读到的计数器值

void Encoder_Init(void){
    HAL_TIM_Encoder_Start(&htim2,TIM_CHANNEL_ALL);//启动定时器
    s_last=(int16_t)__HAL_TIM_GET_COUNTER(&htim2);//记录当前值
    s_position=0;//清零
}

int32_t Encoder_ReadDelta(void){
    int16_t now=(int16_t)__HAL_TIM_GET_COUNTER(&htim2);
    int16_t delta=now-s_last;//现在的计数=当前计数-最早记录的当前值
    s_last=now;//更新基准
    s_position += delta;//累加到总位置
    return delta;//返回这次的增量
} 

int32_t Encoder_GetPosition(void){
    return s_position;
}



