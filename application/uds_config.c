#include "uds.h"
#include "main.h"
#include "abs_tim.h"
#include "stm32c0xx_hal.h"
#include <stdio.h>
#include <assert.h>
#include "addr.h"
#include "uds_config.h"

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
	if(new_sess == UDS_DIAG_SESS_PROG) {
		*ADDR_BL_FLAG_PTR = ADDR_BL_FLAG_SWITCH_PROG_SESS;
		HAL_NVIC_SystemReset();
		while (1) {
			__asm__("nop");
		}
	}
}

static void uds_ecu_reset(uint8_t reset_type)
{
	printf("reset:%d\n", (int)reset_type);
	HAL_NVIC_SystemReset();
	while (1) {
		__asm__("nop");
	}
}

static void uds_security_level_on_changed(uint8_t new_level)
{
	printf("level:%d\n", (int)new_level);
	return;
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

typedef struct {
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
	uint8_t reserved;
} sw_version_s;

static sw_version_s _uds_impl_version = {
	.major = UDS_MAJOR_VERSION,
	.minor = UDS_MINOR_VERSION,
	.patch = UDS_PATCH_VERSION,
	.reserved = 0
};

static uint16_t _blink_delay_ms = 150;

uint16_t uds_config_get_blink_delay_ms(void)
{
	return _blink_delay_ms;
}

static uds_did_s _did_arr[] = {
	{// uds implementation version
		.did = 0x2025,
		.buf_ptr = (uint8_t *)&_uds_impl_version,
		.buf_size = sizeof(_uds_impl_version),
		.write_access = false,
		.req_security_level = 0,
		.diag_sess_ptr = _all_diag_sess_arr,
		.num_diag_sess = sizeof(_all_diag_sess_arr)
	},
	{
		.did = 0x2026,
		.buf_ptr = (uint8_t *)&_blink_delay_ms,
		.buf_size = sizeof(_blink_delay_ms),
		.write_access = true,
		.req_security_level = SECURITY_ACCESS_CALIB_SEED,
		.diag_sess_ptr = _all_diag_sess_arr,
		.num_diag_sess = sizeof(_all_diag_sess_arr)
	}
};

static uds_dtc_s _dtc_arr[] = {
	{
		.status.r = 0,
		.id = {
			.high = 0x81,
			.mid = 0x23,
			.low = 0x9e
		}
	}
};

uds_cfg_s _uds_cfg = {
	.is_serv_en = {
		.diag_sess_ctrl= true,
		.tester_present= true,
		.ecu_reset= true,
		.security_access= true,
		.routine_ctrl= false,
		.write_data_by_id= true,
		.routine_download= false,
		.req_transfer_exit= false,
		.transfer_data= false,
		.read_data_by_id= true,
		.read_dtc_info= true
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

	.rid_ptr = NULL,
	.num_rid = -1,

	.did_ptr = _did_arr,
	.num_did = sizeof(_did_arr) / sizeof(uds_did_s),

	.startup_diag_sess = UDS_DIAG_SESS_DEFAULT,
	.startup_security_level = 0,

	.generate_pos_resp_prog = false,
	.generate_pos_resp_extd = false,

	.dtc_ptr = _dtc_arr,
	.num_dtc = sizeof(_dtc_arr) / sizeof(uds_dtc_s),
};

uds_handle_s _uds_handle;
