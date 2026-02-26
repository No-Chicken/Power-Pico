#include "beep.h"
#include "tim.h"

void beep_open(void) {
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4); // 启动PWM通道4用于蜂鸣器
}

void beep_close(void) {
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4); // 停止PWM通道4
}
