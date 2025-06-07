#include "main.h"
#include "hw.h"
#include <stdint.h>
#include <stdio.h>
#include "uds.h"
#include "addr.h"
#include "uds_config.h"

#define UDS_RESP_ID (0x761)
#define UDS_REQ_ID (0x760)

typedef enum {
	LED_BLINK_BLUE,
	LED_BLINK_GREEN,
	LED_BLINK_BOTH,
	LED_BLINK_COUNT
} led_blink_e;

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

static led_blink_e get_led_to_blink(void)
{
	static uint64_t btn_timestamp = 0;
	static bool btn_prev_st = false;
#if defined(USE_LED_BLUE)
	static led_blink_e led_blink = LED_BLINK_BLUE;
#elif defined(USE_LED_GREEN)
	static led_blink_e led_blink = LED_BLINK_GREEN;
#endif
	bool btn_st = hw_read_button();

	if(
		(btn_st == true) && // button pressed
		(btn_st != btn_prev_st)) {
		led_blink++;
		if(led_blink >= LED_BLINK_COUNT) {
			led_blink = LED_BLINK_BLUE;
		}

		btn_timestamp = abs_tim_get();
	} else if(
		(btn_st == true) &&
		(btn_st == btn_prev_st)
	) { // still holding the button
		// if button was pressed for more than 5 seconds, imagine that button was stuck
		if(abs_tim_get_elapsed(btn_timestamp) > 5000) {
			// trigger a DTC here
			led_blink = LED_BLINK_COUNT;
			uds_set_dtc_st(
				0, // DTC index, 0 is the first DTC
				true // set the DTC as triggered
			);
		}
	}

	btn_prev_st = btn_st;

	return led_blink;
}

static void led_blink_handler(void)
{
	static uint64_t led_timestamp = 0;
	static bool led_st = true;
	// blink
	if(abs_tim_get_elapsed(led_timestamp) >= uds_config_get_blink_delay_ms()) {
		led_timestamp = abs_tim_get();
		led_st = !led_st; // toggle led state

		switch(get_led_to_blink()) {
		case LED_BLINK_BLUE:
			hw_led_blue_set(led_st);
			hw_led_green_set(false);
			break;
		case LED_BLINK_GREEN:
			hw_led_blue_set(false);
			hw_led_green_set(led_st);
			break;
		case LED_BLINK_BOTH:
			// both in same state in same time
			hw_led_blue_set(led_st);
			hw_led_green_set(led_st);
			break;
		default:
			// switch between leds
			hw_led_blue_set(!led_st);
			hw_led_green_set(led_st);
			break;
		}
	}
}

static void nvm_init(void)
{
	volatile uint32_t *nvm_uniq_ptr = (uint32_t *)(
		ADDR_RAM_NVM +
		ADDR_RAM_NVM_LENGTH -
		4
	);

	(void)memcpy(
		(void *)ADDR_RAM_NVM,
		(void *)ADDR_NVM,
		ADDR_RAM_NVM_LENGTH
	);

	if((*nvm_uniq_ptr) != ADDR_NVM_LAST_UNIQ_WORD) {
		(void)memset(
			(void *)ADDR_RAM_NVM,
			0,
			ADDR_RAM_NVM_LENGTH
		);
		*nvm_uniq_ptr = ADDR_NVM_LAST_UNIQ_WORD;
	}
}

static void nvm_update(void)
{
	static FLASH_EraseInitTypeDef flash_erase_init_type = {
		.TypeErase = FLASH_TYPEERASE_PAGES,
		.Page = ((uint32_t)ADDR_NVM - ADDR_FLASH) / FLASH_PAGE_SIZE,
		.NbPages = 1
	};
	uint32_t page_err = 0xFFFFFFFFU;
	bool is_different = false;
	volatile uint64_t *u64_ptr = NULL;

	is_different = memcmp(
		(void *)ADDR_RAM_NVM,
		(void *)ADDR_NVM,
		ADDR_RAM_NVM_LENGTH
	) == 0 ? false : true;

	if(is_different) {
		HAL_FLASH_Unlock();
		__HAL_FLASH_CLEAR_FLAG(
			FLASH_FLAG_EOP |
			FLASH_FLAG_PGAERR |
			FLASH_FLAG_WRPERR |
			FLASH_FLAG_OPTVERR
		);
		HAL_FLASHEx_Erase(&flash_erase_init_type, &page_err);
		u64_ptr = (uint64_t *)ADDR_RAM_NVM;
		for(int i = 0; i < (ADDR_RAM_NVM_LENGTH/sizeof(uint64_t)); ++i) {
			HAL_FLASH_Program(
				FLASH_TYPEPROGRAM_DOUBLEWORD,
				ADDR_NVM + (sizeof(uint64_t) * i),
				*u64_ptr
			);
			u64_ptr++;
		}
		HAL_FLASH_Lock();
	}
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

	HAL_TIM_Base_Start(&_hw.htim14);

	nvm_init();
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

	while(1) {
		nvm_update();

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

		led_blink_handler();
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
