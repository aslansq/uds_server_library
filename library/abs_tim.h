#ifndef ABS_TIM_H
#define ABS_TIM_H

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t  (*abs_tim_u8_get_func_ptr_t )(void);
typedef uint16_t (*abs_tim_u16_get_func_ptr_t)(void);
typedef uint32_t (*abs_tim_u32_get_func_ptr_t)(void);

typedef enum {
	ABS_TIM_HW_TIM_0_BIT = 0, // to represent invalid configuration
	ABS_TIM_HW_TIM_8_BIT,
	ABS_TIM_HW_TIM_16_BIT,
	ABS_TIM_HW_TIM_24_BIT,
	ABS_TIM_HW_TIM_32_BIT,
	ABS_TIM_HW_TIM_COUNT
} abs_tim_hw_tim_e;

typedef struct {
	void *get_func_ptr; // Function pointer to get the current time
	abs_tim_hw_tim_e hw_type; // Type of hardware timer
} abs_tim_cfg_s;

typedef struct {
	abs_tim_cfg_s *cfg_ptr;
	uint64_t init_time;
	uint64_t ovf_count;
	bool is_init;
} abs_tim_handle_s;

void abs_tim_x_init(
	abs_tim_handle_s *handle_ptr,
	abs_tim_cfg_s *cfg_ptr
);

uint64_t abs_tim_x_get(const abs_tim_handle_s *handle_ptr);

uint64_t abs_tim_x_get_elapsed(
	const abs_tim_handle_s *handle_ptr,
	uint64_t timestamp
);

void abs_tim_x_ovf(abs_tim_handle_s *handle_ptr);

void abs_tim_init(void);
uint64_t abs_tim_get(void);
uint64_t abs_tim_get_elapsed(uint64_t timestamp);
void abs_tim_ovf(void);

extern abs_tim_handle_s _abs_tim;
extern abs_tim_cfg_s _abs_tim_cfg;


#endif
