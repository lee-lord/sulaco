#include <stdint.h>

#include "platform.h"
#include "drivers/io.h"

#include "drivers/timer.h"
#include "drivers/timer_def.h"
#include "drivers/dma.h"

const timerHardware_t timerHardware[USABLE_TIMER_CHANNEL_COUNT] = {
    DEF_TIM(TIM8,  CH2, PC7,  TIM_USE_PWM | TIM_USE_PPM, 0, 0 ), // PPM IN
    DEF_TIM(TIM12, CH1, PB14, TIM_USE_PWM,               0, 0 ), // S2_IN
    DEF_TIM(TIM12, CH2, PB15, TIM_USE_PWM,               0, 0 ), // S3_IN - GPIO_PartialRemap_TIM3
    DEF_TIM(TIM8,  CH3, PC8,  TIM_USE_PWM,               0, 0 ), // S4_IN
    DEF_TIM(TIM8,  CH4, PC9,  TIM_USE_PWM,               0, 0 ), // S5_IN
    DEF_TIM(TIM3,  CH3, PB0,  TIM_USE_MOTOR,             0, 0 ), // S1_OUT
    DEF_TIM(TIM3,  CH4, PB1,  TIM_USE_MOTOR,             0, 0 ), // S2_OUT
    DEF_TIM(TIM2,  CH4, PA3,  TIM_USE_MOTOR,             0, 0 ), // S3_OUT
    DEF_TIM(TIM2,  CH3, PA2,  TIM_USE_MOTOR,             0, 0 ), // S4_OUT
    DEF_TIM(TIM5,  CH2, PA1,  TIM_USE_MOTOR,             0, 0 ), // S5_OUT - GPIO_PartialRemap_TIM3
    DEF_TIM(TIM5,  CH1, PA0,  TIM_USE_MOTOR,             0, 0 ), // S6_OUT
};
