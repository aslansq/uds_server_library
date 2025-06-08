#ifndef UDS_CONFIG_H
#define UDS_CONFIG_H

#include <stdint.h>

typedef enum {
	UDS_CONFIG_DTC_IDX_BUTTON_STUCK = 0u,
	UDS_CONFIG_DTC_IDX_COUNT
} uds_config_dtc_idx_e;

uint16_t uds_config_get_blink_delay_ms(void);

#endif // UDS_CONFIG_H