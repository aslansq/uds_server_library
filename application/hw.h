#ifndef HW_H
#define HW_H

#include "stm32c0xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
	UART_HandleTypeDef huart1;
	FDCAN_HandleTypeDef hfdcan1;
	TIM_HandleTypeDef htim14;
} hw_s;

void hw_led_green_set(bool on);
void hw_led_blue_set(bool on);
bool hw_read_button(void);

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART1_UART_Init(void);
void MX_FDCAN1_Init(void);
void MX_TIM14_Init(void);

extern hw_s _hw;

#endif // HW_H