#include "main.h"
#include "hw.h"
#include <stdint.h>
#include <stdio.h>
#include "uds.h"
#include "addr.h"

#define UDS_RESP_ID (0x761)
#define UDS_REQ_ID (0x760)

ecu_handle_s _ecu_handle = {
	.tx_header = {
		.Identifier          = UDS_RESP_ID,
		.IdType              = FDCAN_STANDARD_ID,
		.TxFrameType         = FDCAN_DATA_FRAME,
		.DataLength          = FDCAN_DLC_BYTES_2,
		.ErrorStateIndicator = FDCAN_ESI_ACTIVE,
		.BitRateSwitch       = FDCAN_BRS_OFF,
		.FDFormat            = FDCAN_CLASSIC_CAN,
		.TxEventFifoControl  = FDCAN_NO_TX_EVENTS,
		.MessageMarker       = 0u
	}
};

void isotp_user_debug(const char* message, ...)
{
	return;
}

int  isotp_user_send_can(
	const uint32_t arbitration_id,
	const uint8_t* data,
	const uint8_t size
)
{
	uint32_t free_elements;
	_ecu_handle.tx_header.DataLength = size;
	HAL_FDCAN_AddMessageToTxFifoQ(&_hw.hfdcan1, &_ecu_handle.tx_header, data);

	do {
		free_elements = HAL_FDCAN_GetTxFifoFreeLevel(&_hw.hfdcan1);
	} while(free_elements == 0u);

	return ISOTP_RET_OK;
}

uint32_t isotp_user_get_ms(void)
{
	return HAL_GetTick();
}

static void can_setup(void)
{
	FDCAN_FilterTypeDef sFilterConfig;
	sFilterConfig.IdType       = FDCAN_STANDARD_ID;
	sFilterConfig.FilterIndex  = 0U;
	sFilterConfig.FilterType   = FDCAN_FILTER_MASK;
	sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	sFilterConfig.FilterID1    = UDS_REQ_ID;
	sFilterConfig.FilterID2    = 0x7FF;
	HAL_FDCAN_ConfigFilter(&_hw.hfdcan1, &sFilterConfig);

	HAL_FDCAN_ConfigGlobalFilter(
		&_hw.hfdcan1,
		FDCAN_REJECT, FDCAN_REJECT,
		FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE
	);

	/* Activate Rx FIFO 0 new message notification */
	HAL_FDCAN_ActivateNotification(&_hw.hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0U);

	/* Start FDCAN controller */
	HAL_FDCAN_Start(&_hw.hfdcan1);
}

int main(void)
{
	uint8_t payload_arr[255] = {0};
	uint16_t out_size = 0;
	int ret;

	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_USART1_UART_Init();
	MX_FDCAN1_Init();
	MX_TIM14_Init();

	can_setup();
	isotp_init_link(
		&_ecu_handle.isotp_link,
		UDS_RESP_ID,
		_ecu_handle.isotp_tx_arr,
		sizeof(_ecu_handle.isotp_tx_arr),
		_ecu_handle.isotp_rx_arr,
		sizeof(_ecu_handle.isotp_rx_arr)
	);
	abs_tim_init();

	if(*ADDR_BL_FLAG_PTR == ADDR_BL_FLAG_SWITCH_EXTD_SESS) {
		*ADDR_BL_FLAG_PTR = ADDR_BL_FLAG_NONE;
		_uds_cfg.generate_pos_resp_extd = true;
		_uds_cfg.startup_diag_sess = UDS_DIAG_SESS_EXT_DIAG;
	}

	uds_init();

	printf("Application started\n");
	__enable_irq();
	uint64_t led_timestamp = abs_tim_get();

	while(1) {
		isotp_poll(&_ecu_handle.isotp_link);
		ret = isotp_receive(
			&_ecu_handle.isotp_link,
			payload_arr,
			sizeof(payload_arr),
			&out_size
		);
		if(ret == ISOTP_RET_OK) {
			uds_put_packet_in(payload_arr, out_size);
		}
		uds_handler();

		// blink
		if(abs_tim_get_elapsed(led_timestamp) >= 50) {
			led_timestamp = abs_tim_get();
#ifdef USE_LED_BLUE
			HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_9);
#endif
#ifdef USE_LED_GREEN
			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
#endif
		}
	}
	return 0;
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
	if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0U) {
		return;
	}
	if(HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &_ecu_handle.rx_header, _ecu_handle.rx_data_arr) != HAL_OK) {
		//Error_Handler();
	} else if(
		(_ecu_handle.rx_header.Identifier == UDS_REQ_ID) &&
		(_ecu_handle.rx_header.IdType     == FDCAN_STANDARD_ID)
	) {
		isotp_on_can_message(
			&_ecu_handle.isotp_link,
			_ecu_handle.rx_data_arr,
			_ecu_handle.rx_header.DataLength
		);
	}
}

int __io_putchar(int ch)
{
	HAL_UART_Transmit(&_hw.huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
	return ch;
}
