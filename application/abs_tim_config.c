#include "stm32c092xx.h"
#include "abs_tim_config.h"
#include "stm32c0xx_hal.h"

uint16_t abs_tim_u16_get(void)
{
	return TIM14->CNT;
}

abs_tim_cfg_s _abs_tim_cfg = {
	.get_func_ptr = abs_tim_u16_get,
	.hw_type = ABS_TIM_HW_TIM_16_BIT
};

abs_tim_handle_s _abs_tim;

