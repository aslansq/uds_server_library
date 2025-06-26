#ifndef UDS_CONFIG_H
#define UDS_CONFIG_H

#include <stdint.h>

typedef enum {
	UDS_CONFIG_DID_IDX_IMPL_VERSION = 0u,
	UDS_CONFIG_DID_IDX_BLINK_DELAY,
	UDS_CONFIG_DID_IDX_ON_TIME,
	UDS_CONFIG_DID_IDX_COUNT
} uds_config_did_idx_e;

typedef enum {
	UDS_CONFIG_DTC_IDX_BUTTON_STUCK = 0u,
	UDS_CONFIG_DTC_IDX_COUNT
} uds_config_dtc_idx_e;

uint16_t uds_config_get_blink_delay_ms(void);
void uds_config_set_ecu_on_time_ms(uint64_t on_time_ms);

#endif // UDS_CONFIG_H
