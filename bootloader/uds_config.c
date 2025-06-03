#include "uds.h"
#include "main.h"
#include "abs_tim.h"
#include "stm32c0xx_hal.h"
#include <stdio.h>
#include <assert.h>
#include "addr.h"

#define UDS_PACKET_RX_BUF_LEN 255
#define UDS_PACKET_TX_BUF_LEN 32

//! Security subfunction id for seed req for calibration
//! This subfunction id also re-presents security level
#define SECURITY_ACCESS_CALIB_SEED 0x01
//! Security subfunction id for send key for calibration
#define SECURITY_ACCESS_CALIB_KEY 0x02
//! Security subfunction id for seed req for programming
//! This subfunction id also re-presents security level
#define SECURITY_ACCESS_PROG_SEED 0x03
//! Security subfunction id for send key for programming
#define SECURITY_ACCESS_PROG_KEY 0x04
//! See linker script. Size of application
#define KB192 (1024*192)

//! Bootloader flag
uint32_t _bl_flag __attribute__((section(".bl_flag_s")));

static void uds_diag_sess_on_changed(uint8_t new_sess)
{
	printf("sess:%d\n", (int)new_sess);
	switch(new_sess) {
	case UDS_DIAG_SESS_PROG:
		break;
	case UDS_DIAG_SESS_EXT_DIAG:
	case UDS_DIAG_SESS_DEFAULT:
	default:
		_bl_flag = 0;
		break;
	}
	return;
}

static void uds_ecu_reset(uint8_t reset_type)
{
	printf("reset:%d\n", (int)reset_type);
	HAL_NVIC_SystemReset();
	while (1) {
		__asm__("nop");
	}
	return;
}

static void uds_security_level_on_changed(uint8_t new_level)
{
	printf("level:%d\n", (int)new_level);
	return;
}

static void uds_rid_check_prog_precond(void *arg_rid_ptr)
{
	uds_rid_s *rid_ptr = (uds_rid_s *)arg_rid_ptr;
	switch (rid_ptr->curr_state) {
	case UDS_RID_STATE_START:
		printf("start precond\n");
		rid_ptr->curr_state = UDS_RID_STATE_RUNNING;
		break;
	case UDS_RID_STATE_RUNNING:
		printf("done precond\n");
		rid_ptr->curr_state = UDS_RID_STATE_DONE;
		break;
	default:
		break;
	}
}

static void uds_rid_erase_flash(void *arg_rid_ptr)
{
	uds_rid_s *rid_ptr = (uds_rid_s *)arg_rid_ptr;
	switch (rid_ptr->curr_state) {
	case UDS_RID_STATE_START:
		printf("start erase\n");
		rid_ptr->curr_state = UDS_RID_STATE_RUNNING;
		break;
	case UDS_RID_STATE_RUNNING:
		printf("done erase\n");
		rid_ptr->curr_state = UDS_RID_STATE_DONE;
		break;
	default:
		break;
	}
}

static void uds_rid_check_mem(void *arg_rid_ptr)
{
	uds_rid_s *rid_ptr = (uds_rid_s *)arg_rid_ptr;
	switch (rid_ptr->curr_state) {
	case UDS_RID_STATE_START:
		printf("start mem\n");
		rid_ptr->curr_state = UDS_RID_STATE_RUNNING;
		break;
	case UDS_RID_STATE_RUNNING:
		printf("done mem\n");
		rid_ptr->curr_state = UDS_RID_STATE_DONE;
		break;
	default:
		break;
	}
}

static void uds_rid_check_prog_dependency(void *arg_rid_ptr)
{
	uds_rid_s *rid_ptr = (uds_rid_s *)arg_rid_ptr;
	switch (rid_ptr->curr_state) {
	case UDS_RID_STATE_START:
		printf("start depen\n");
		rid_ptr->curr_state = UDS_RID_STATE_RUNNING;
		break;
	case UDS_RID_STATE_RUNNING:
		printf("done depen\n");
		rid_ptr->curr_state = UDS_RID_STATE_DONE;
		break;
	default:
		break;
	}
}

static bool _is_page_erased[KB192 / FLASH_PAGE_SIZE] = {0};

static bool uds_routine_download(void *arg_handle_ptr)
{
	(void)arg_handle_ptr; // Unused parameter
	memset(
		_is_page_erased,
		0,
		sizeof(_is_page_erased)
	);
	return true; // Indicate success
}

static void uds_req_transfer_exit(void)
{
	HAL_FLASH_Lock();
	printf("exit\n");
}

static FLASH_EraseInitTypeDef _flash_erase_init_type = {
	.TypeErase = FLASH_TYPEERASE_PAGES,
	.Page = 0,
	.NbPages = 1
};

static bool uds_transfer_data_erase_page(uds_transfer_data_s *transfer_data_ptr)
{
	uint32_t page_idx_offset = ((uint32_t)ADDR_APP - ADDR_FLASH) / FLASH_PAGE_SIZE;
	uint32_t page_idx = (transfer_data_ptr->mem_addr - ADDR_FLASH) / FLASH_PAGE_SIZE;
	uint32_t is_page_erased_idx = page_idx - page_idx_offset;
	uint32_t page_err = 0xFFFFFFFFU;

	if(is_page_erased_idx >= sizeof(_is_page_erased) / sizeof(_is_page_erased[0])) {
		printf("Error: Page index out of bounds\n");
		return false;
	} else {
		if(!_is_page_erased[is_page_erased_idx]) {
			_flash_erase_init_type.Page = page_idx;
			// Erase the page if it is not erased yet
			HAL_FLASH_Unlock();
			if(FLASH->CR & FLASH_CR_LOCK) {
				printf("lock\n");
				return false;
			} else {
				printf("unlock\n");
			}
			__HAL_FLASH_CLEAR_FLAG(
				FLASH_FLAG_EOP |
				FLASH_FLAG_PGAERR |
				FLASH_FLAG_WRPERR |
				FLASH_FLAG_OPTVERR
			);
			HAL_FLASHEx_Erase(&_flash_erase_init_type, &page_err);
			HAL_FLASH_Lock();
			if(page_err != 0xFFFFFFFFU) {
				printf("Error: Page erase failed at index %lu\n", (unsigned long)page_idx);
				return false;
			} else {
				_is_page_erased[is_page_erased_idx] = true;
				printf("Page %lu erased\n", (unsigned long)page_idx);
			}
		}
	}
	return true; // Indicate success
}

static void uds_transfer_data(void *arg_handle_ptr)
{
	uint64_t u64;
	volatile uint64_t *u64_ptr = NULL;
	uint32_t addr = 0;
	uds_transfer_data_s *transfer_data_ptr = (uds_transfer_data_s *)arg_handle_ptr;
	if (transfer_data_ptr == NULL) {
		return;
	}

	if(
		(transfer_data_ptr->data_ptr == NULL) ||
		(transfer_data_ptr->recv_size == 0)
	) {
		printf("Error: Invalid transfer data\n");
		return;
	}

	if(uds_transfer_data_erase_page(transfer_data_ptr) == false) {
		return;
	}

	int64_t app_idx = transfer_data_ptr->mem_addr - ADDR_APP;

	if(app_idx < 0) {
		printf("Error: Invalid mem addr transfer data\n");
		return;
	}

	printf(
		"address: %lx recv size: %lu bsc:%lu\n",
		(unsigned long)transfer_data_ptr->mem_addr,
		(unsigned long)transfer_data_ptr->recv_size,
		(unsigned long)transfer_data_ptr->bsc
	);

	for(uint32_t i = 0; i < transfer_data_ptr->recv_size;) {
		u64 = 0;
		uint8_t j;
		for(
			j = 0;
			j < 8 &&
			i < transfer_data_ptr->recv_size
			; j++, i++
		) {
			u64 |= ((uint64_t)transfer_data_ptr->data_ptr[i] << (j * 8));
		}

		if(u64 == UINT64_MAX) {
			continue; // Skip writing if the data is all 0xFF
		}

		addr = transfer_data_ptr->mem_addr + i - j;
		HAL_FLASH_Unlock();
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, u64);
		HAL_FLASH_Lock();
		u64_ptr = (volatile uint64_t *)(addr);

		if(*u64_ptr != u64) {
			printf("Error: Flash write failed at address %lx\n", addr);
			return;
		}
	}
}

void uds_sec_acc_calc(
	uint8_t level,
	uint8_t *seed_ptr,
	uint8_t *key_ptr,
	uint8_t seed_size,
	uint8_t key_size
)
{
	assert(seed_ptr != NULL);
	assert(key_ptr != NULL);
	assert(seed_size > 0);
	assert(key_size > 0);
	assert(seed_size == key_size);

	for(int i = 0; i < seed_size; ++i) {
		int j = (seed_size - 1) - i;
		key_ptr[j] = seed_ptr[i] + level;
	}
}

void uds_sec_acc_get_seed(
	uint8_t level,
	uint8_t *seed_ptr,
	uint8_t seed_size
)
{
	assert(seed_ptr != NULL);
	assert(seed_size > 0);

	for(int i = 0; i < seed_size; i++) {
		seed_ptr[i] = level + i;
	}
}

static uint8_t _uds_rx_packet_arr[UDS_PACKET_RX_BUF_LEN] = {0};
static uint8_t _uds_tx_packet_arr[UDS_PACKET_TX_BUF_LEN] = {0};

static uds_diag_sess_s _available_diag_sess_arr[] = {
	{
		.diag_sess = UDS_DIAG_SESS_DEFAULT,
		.req_security_level = 0
	},
	{
		.diag_sess = UDS_DIAG_SESS_PROG,
		.req_security_level = 0
	},
	{
		.diag_sess = UDS_DIAG_SESS_EXT_DIAG,
		.req_security_level = 0
	}
};

static uint8_t _all_diag_sess_arr[] = {
	UDS_DIAG_SESS_DEFAULT,
	UDS_DIAG_SESS_PROG,
	UDS_DIAG_SESS_EXT_DIAG
};

static uds_ecu_reset_s _available_ecu_reset_arr[] = {
	{
		.reset_type = UDS_RESET_TYPE_HARD,
		.req_security_level = 0,
		.diag_sess_ptr = _all_diag_sess_arr,
		.num_diag_sess = sizeof(_all_diag_sess_arr)
	}
};

static uint8_t _sec_acc_level_def_seed_arr[6] = {0};
static uint8_t _sec_acc_level_def_key_arr[6] = {0};
static uint8_t _sec_acc_level_seed_arr[6] = {0};
static uint8_t _sec_acc_level_key_arr[6] = {0};

uds_security_access_s _sec_acc_arr[] = {
	{
		.seed_level = SECURITY_ACCESS_CALIB_SEED,
		.key_level = SECURITY_ACCESS_CALIB_KEY,
		.seed_ptr = _sec_acc_level_def_seed_arr,
		.key_ptr = _sec_acc_level_def_key_arr,
		.seed_size = sizeof(_sec_acc_level_def_seed_arr),
		.key_size = sizeof(_sec_acc_level_def_key_arr),
		.diag_sess_ptr = _all_diag_sess_arr,
		.num_diag_sess = sizeof(_all_diag_sess_arr)
	},
	{
		.seed_level = SECURITY_ACCESS_PROG_SEED,
		.key_level = SECURITY_ACCESS_PROG_KEY,
		.seed_ptr = _sec_acc_level_seed_arr,
		.key_ptr = _sec_acc_level_key_arr,
		.seed_size = sizeof(_sec_acc_level_seed_arr),
		.key_size = sizeof(_sec_acc_level_key_arr),
		.diag_sess_ptr = _all_diag_sess_arr,
		.num_diag_sess = sizeof(_all_diag_sess_arr)
	}
};

static uint8_t _rid_check_prog_precond_result_arr[1] = {0};
static uint8_t _rid_erase_flash_result_arr[1] = {0};
static uint8_t _rid_check_memory_result_arr[1] = {0};
static uint8_t _rid_check_prog_dependency_result_arr[1] = {0};
static uint8_t _rid_erase_flash_arg_arr[9] = {0};


static uds_rid_s _rid_arr[] = {
	{ // check programming preconditions
		.curr_state = UDS_RID_STATE_IDLE,
		.id = (uint16_t)0x0203,
		.argument_ptr = NULL,
		.argument_size = 0,
		.result_ptr = _rid_check_prog_precond_result_arr,
		.result_size = sizeof(_rid_check_prog_precond_result_arr),
		.req_security_level = 0,
		.diag_sess_ptr = _all_diag_sess_arr,
		.num_diag_sess = sizeof(_all_diag_sess_arr),
		.run_timeout_ms = 5000, // 5 seconds
		.func_ptr = uds_rid_check_prog_precond, // user function to check preconditions
		.start_time_ms = 0
	},
	{ // erase flash
		.curr_state = UDS_RID_STATE_IDLE,
		.id = (uint16_t)0xff00,
		.argument_ptr = _rid_erase_flash_arg_arr,
		.argument_size = sizeof(_rid_erase_flash_arg_arr),
		.result_ptr = _rid_erase_flash_result_arr,
		.result_size = sizeof(_rid_erase_flash_result_arr),
		.req_security_level = 3,
		.diag_sess_ptr = _all_diag_sess_arr,
		.num_diag_sess = sizeof(_all_diag_sess_arr),
		.run_timeout_ms = 5000, // 5 seconds
		.func_ptr = uds_rid_erase_flash, // user function to erase flash
		.start_time_ms = 0
	},
	{ // check memory
		.curr_state = UDS_RID_STATE_IDLE,
		.id = (uint16_t)0x0202,
		.argument_ptr = NULL,
		.argument_size = 0,
		.result_ptr = _rid_check_memory_result_arr, // Reusing the same result array for simplicity
		.result_size = sizeof(_rid_check_memory_result_arr),
		.req_security_level = 3,
		.diag_sess_ptr = _all_diag_sess_arr,
		.num_diag_sess = sizeof(_all_diag_sess_arr),
		.run_timeout_ms = 5000, // 5 seconds
		.func_ptr = uds_rid_check_mem, // user function to check memory
		.start_time_ms = 0
	},
	{ // check programming dependencies
		.curr_state = UDS_RID_STATE_IDLE,
		.id = (uint16_t)0xff01,
		.argument_ptr = NULL,
		.argument_size = 0,
		.result_ptr = _rid_check_prog_dependency_result_arr, // Reusing the same result array for simplicity
		.result_size = sizeof(_rid_check_prog_dependency_result_arr),
		.req_security_level = 3,
		.diag_sess_ptr = _all_diag_sess_arr,
		.num_diag_sess = sizeof(_all_diag_sess_arr),
		.run_timeout_ms = 5000, // 5 seconds
		.func_ptr = uds_rid_check_prog_dependency, // user function to check dependencies
		.start_time_ms = 0
	}
};

static uint8_t _did_flasher_fingerprint_arr[3] = {0};
static uds_did_s _did_arr[] = {
	{// flasher fingerprint
		.did = 0xf15a,
		.buf_ptr = _did_flasher_fingerprint_arr,
		.buf_size = sizeof(_did_flasher_fingerprint_arr),
		.write_access = true,
		.req_security_level = 3,
		.diag_sess_ptr = _all_diag_sess_arr,
		.num_diag_sess = sizeof(_all_diag_sess_arr)
	}
};

static uint8_t uds_transfer_data_arr[128]; // Buffer for transfer data

uds_cfg_s _uds_cfg = {
	.is_serv_en = {
		.diag_sess_ctrl= true,
		.tester_present= true,
		.ecu_reset= true,
		.security_access= true,
		.routine_ctrl= true,
		.write_data_by_id= true,
		.routine_download= true,
		.req_transfer_exit= true,
		.transfer_data= true,
		.read_data_by_id= true
	},

	// it should be able to hold single uds packet.
	// since uds packets are on isotp, their size could be bigger than CAN DLC.
	.rx_ptr = _uds_rx_packet_arr,
	.rx_buf_size = sizeof(_uds_rx_packet_arr),
	// it should be able to hold single uds packet.
	// since uds packets are on isotp, their size could be bigger than CAN DLC.
	.tx_ptr = _uds_tx_packet_arr,
	.tx_buf_size = sizeof(_uds_tx_packet_arr),

	.iso_tp_handle_ptr = &_ecu_handle.isotp_link,
	.iso_tp_send_func_ptr = (uds_iso_tp_send_func_t)isotp_send,

	.avail_diag_sess_ptr = _available_diag_sess_arr,
	.num_avail_diag_sess = sizeof(_available_diag_sess_arr) / sizeof(uds_diag_sess_s),
	.diag_sess_cbk_ptr = uds_diag_sess_on_changed,

	.p2_server_max = 2000,
	.p2_star_server_max = 200,

	.ecu_reset_func_ptr = uds_ecu_reset,
	.ecu_reset_ptr = _available_ecu_reset_arr,
	.num_ecu_reset = sizeof(_available_ecu_reset_arr) / sizeof(uds_ecu_reset_s),

	.sec_acc_cbk_ptr = uds_security_level_on_changed,
	.sec_acc_ptr = _sec_acc_arr,
	.num_sec_acc = sizeof(_sec_acc_arr) / sizeof(uds_security_access_s),
	.sec_acc_calc_func_ptr = uds_sec_acc_calc,
	.sec_acc_get_seed_func_ptr = uds_sec_acc_get_seed,

	.abs_tim_handle_ptr = &_abs_tim,

	.rid_ptr = _rid_arr,
	.num_rid = sizeof(_rid_arr) / sizeof(uds_rid_s),

	.did_ptr = _did_arr,
	.num_did = sizeof(_did_arr) / sizeof(uds_did_s),

	.routine_download = {
		.cbk_ptr = uds_routine_download,
		.mem_addr = 0,
		.mem_size = 0,
		.mem_addr_len = 0,
		.block_size = sizeof(uds_transfer_data_arr) / 2, // normally it can be equal just to be in safe side
		.req_security_level = 3,
		.diag_sess_ptr = _all_diag_sess_arr,
		.num_diag_sess = sizeof(_all_diag_sess_arr)
	},

	.req_transfer_exit = {
		.cbk_ptr = uds_req_transfer_exit,
		.req_security_level = 3,
		.diag_sess_ptr = _all_diag_sess_arr,
		.num_diag_sess = sizeof(_all_diag_sess_arr)
	},

	.transfer_data = {
		.data_ptr = uds_transfer_data_arr,
		.data_size = sizeof(uds_transfer_data_arr),
		.recv_size = 0, // Initial size is 0, will be updated during transfer
		.cbk_ptr = uds_transfer_data,
		.req_security_level = 3, // Minimum required security level for this request transfer exit
		.diag_sess_ptr = _all_diag_sess_arr, // Pointer to the allowed diagnostic sessions for this request transfer exit
		.num_diag_sess = sizeof(_all_diag_sess_arr), // Number of allowed diagnostic sessions for this request transfer exit
		.bsc = 0 // Block sequence counter, initialized to 0
	},

	.startup_diag_sess = UDS_DIAG_SESS_PROG,
	.startup_security_level = SECURITY_ACCESS_PROG_SEED,

	.generate_pos_resp_prog = false,
	.generate_pos_resp_extd = false,
};

uds_handle_s _uds_handle;
