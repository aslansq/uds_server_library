#include <assert.h>
#include "hw.h"
#include "abs_tim.h"

hw_s _hw;

void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	__HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);

	/** Initializes the RCC Oscillators according to the specified parameters
	* in the RCC_OscInitTypeDef structure.
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);

	/** Initializes the CPU, AHB and APB buses clocks
	*/
	RCC_ClkInitStruct.ClockType =
		RCC_CLOCKTYPE_HCLK |
		RCC_CLOCKTYPE_SYSCLK |
		RCC_CLOCKTYPE_PCLK1;

	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;

	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1);
}

void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);

	/*Configure GPIO pin : PC9 */
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void MX_USART1_UART_Init(void)
{
	_hw.huart1.Instance = USART1;
	_hw.huart1.Init.BaudRate = 115200;
	_hw.huart1.Init.WordLength = UART_WORDLENGTH_8B;
	_hw.huart1.Init.StopBits = UART_STOPBITS_1;
	_hw.huart1.Init.Parity = UART_PARITY_NONE;
	_hw.huart1.Init.Mode = UART_MODE_TX_RX;
	_hw.huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	_hw.huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	_hw.huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	_hw.huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
	_hw.huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	HAL_UART_Init(&_hw.huart1);
	HAL_UARTEx_SetTxFifoThreshold(&_hw.huart1, UART_TXFIFO_THRESHOLD_1_8);
	HAL_UARTEx_SetRxFifoThreshold(&_hw.huart1, UART_RXFIFO_THRESHOLD_1_8);
	HAL_UARTEx_DisableFifoMode(&_hw.huart1);
}

void MX_FDCAN1_Init(void)
{
	_hw.hfdcan1.Instance = FDCAN1;
	_hw.hfdcan1.Init.ClockDivider = FDCAN_CLOCK_DIV1;
	_hw.hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
	_hw.hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
	_hw.hfdcan1.Init.AutoRetransmission = ENABLE;
	_hw.hfdcan1.Init.TransmitPause = ENABLE;
	_hw.hfdcan1.Init.ProtocolException = DISABLE;
	_hw.hfdcan1.Init.NominalPrescaler = 1;
	_hw.hfdcan1.Init.NominalSyncJumpWidth = 12;
	_hw.hfdcan1.Init.NominalTimeSeg1 = 35;
	_hw.hfdcan1.Init.NominalTimeSeg2 = 12;
	_hw.hfdcan1.Init.DataPrescaler = 1;
	_hw.hfdcan1.Init.DataSyncJumpWidth = 6;
	_hw.hfdcan1.Init.DataTimeSeg1 = 17;
	_hw.hfdcan1.Init.DataTimeSeg2 = 6;
	_hw.hfdcan1.Init.StdFiltersNbr = 1;
	_hw.hfdcan1.Init.ExtFiltersNbr = 0;
	_hw.hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
	HAL_FDCAN_Init(&_hw.hfdcan1);
}

void MX_TIM14_Init(void)
{
	_hw.htim14.Instance = TIM14;
	_hw.htim14.Init.Prescaler = 47999;
	_hw.htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
	_hw.htim14.Init.Period = 65535;
	_hw.htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	_hw.htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	HAL_TIM_Base_Init(&_hw.htim14);
}

void SysTick_Handler(void)
{
	HAL_IncTick();
}

void FDCAN1_IT0_IRQHandler(void)
{
	HAL_FDCAN_IRQHandler(&_hw.hfdcan1);
}

//void Reset_Handler(void) {while(1) {__asm__("nop");}}
void NMI_Handler(void) {while(1) {__asm__("nop");}}
void HardFault_Handler(void) {while(1) {__asm__("nop");}}
void SVC_Handler(void) {while(1) {__asm__("nop");}}
void PendSV_Handler(void) {while(1) {__asm__("nop");}}
//void SysTick_Handler(void) {while(1) {__asm__("nop");}}
void WWDG_IRQHandler(void) {while(1) {__asm__("nop");}}
void RTC_IRQHandler(void) {while(1) {__asm__("nop");}}
void FLASH_IRQHandler(void) {while(1) {__asm__("nop");}}
void RCC_IRQHandler(void) {while(1) {__asm__("nop");}}
void EXTI0_1_IRQHandler(void) {while(1) {__asm__("nop");}}
void EXTI2_3_IRQHandler(void) {while(1) {__asm__("nop");}}                /* EXTI Line 2 and 3                           */
void EXTI4_15_IRQHandler(void) {while(1) {__asm__("nop");}}               /* EXTI Line 4 to 15                           */
void DMA1_Channel1_IRQHandler(void) {while(1) {__asm__("nop");}}          /* DMA1 Channel 1                              */
void DMA1_Channel2_3_IRQHandler(void) {while(1) {__asm__("nop");}}        /* DMA1 Channel 2 and Channel 3                */
void DMAMUX1_DMA1_CH4_5_IRQHandler(void) {while(1) {__asm__("nop");}}     /* DMAMUX1, DMA1 Channel 4 and 5               */
void ADC1_IRQHandler(void) {while(1) {__asm__("nop");}}                   /* ADC1                                        */
void TIM1_BRK_UP_TRG_COM_IRQHandler(void) {while(1) {__asm__("nop");}}    /* TIM1 Break, Update, Trigger and Commutation */
void TIM1_CC_IRQHandler(void) {while(1) {__asm__("nop");}}                /* TIM1 Capture Compare                        */
void TIM2_IRQHandler(void) {while(1) {__asm__("nop");}}                   /* TIM2                                        */
void TIM3_IRQHandler(void) {while(1) {__asm__("nop");}}                   /* TIM3                                        */

void TIM14_IRQHandler(void)
{
	__HAL_TIM_CLEAR_FLAG(&_hw.htim14, TIM_FLAG_UPDATE);
	abs_tim_ovf();
}

void TIM15_IRQHandler(void) {while(1) {__asm__("nop");}}                  /* TIM15                                       */
void TIM16_IRQHandler(void) {while(1) {__asm__("nop");}}                  /* TIM16                                       */
void TIM17_IRQHandler(void) {while(1) {__asm__("nop");}}                  /* TIM17                                       */
void I2C1_IRQHandler(void) {while(1) {__asm__("nop");}}                   /* I2C1                                        */
void I2C2_IRQHandler(void) {while(1) {__asm__("nop");}}                   /* I2C1                                        */
void SPI1_IRQHandler(void) {while(1) {__asm__("nop");}}                   /* SPI1                                        */
void SPI2_IRQHandler(void) {while(1) {__asm__("nop");}}                   /* SPI1                                        */
void USART1_IRQHandler(void) {while(1) {__asm__("nop");}}                 /* USART1                                      */
void USART2_IRQHandler(void) {while(1) {__asm__("nop");}}                 /* USART2                                      */
void USART3_4_IRQHandler(void) {while(1) {__asm__("nop");}}               /* USART3 and USART4                           */
//void FDCAN1_IT0_IRQHandler(void) {while(1) {__asm__("nop");}}             /* FDCAN1 interrupt request 0 pending          */
void FDCAN1_IT1_IRQHandler(void) {while(1) {__asm__("nop");}}             /* FDCAN1 interrupt request 1 pending          */