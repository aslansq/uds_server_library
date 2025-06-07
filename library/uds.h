/**
 * @file uds.h
 * @brief UDS (Unified Diagnostic Services) Implementation Header File
 * This file contains the definitions and structures used in the UDS implementation.
 * 
 * This implementation is based on the UDS protocol as defined in ISO 14229-1:2020.
 * 
 * @warning It is mandatory to create the following global variables in your application:
 * uds_cfg_s _uds_cfg;
 * uds_handle_s _uds_handle;
 */

#ifndef UDS_H
#define UDS_H

#include "abs_tim.h"
#include <stdint.h>
#include <stdbool.h>

#define UDS_MAJOR_VERSION 0 //!< Major version of the UDS implementation
#define UDS_MINOR_VERSION 1 //!< Minor version of the UDS implementation
#define UDS_PATCH_VERSION 0 //!< Patch version of the UDS implementation

//! Diagnostic sessions defined by UDS protocol
#define UDS_DIAG_SESS_DEFAULT ((uint8_t)0x01) //!< Default diagnostic session
#define UDS_DIAG_SESS_PROG ((uint8_t)0x02) //!< Programming diagnostic session
#define UDS_DIAG_SESS_EXT_DIAG ((uint8_t)0x03) //!< Extended diagnostic session
#define UDS_DIAG_SESS_SAFETY_SYS_DIAG ((uint8_t)0x04) //!< Safety system diagnostic session

//! Reset types defined by UDS protocol
#define UDS_RESET_TYPE_HARD ((uint8_t)0x01) //!< Hard reset type
#define UDS_RESET_TYPE_KEY_OFF_ON ((uint8_t)0x02) //!< Key off/on reset type
#define UDS_RESET_TYPE_SOFT ((uint8_t)0x03) //!< Soft reset type
#define UDS_RESET_TYPE_EN_RAPID_PWR_SHUT_DOWN ((uint8_t)0x04) //!< Enable rapid power shutdown reset type
#define UDS_RESET_TYPE_DIS_RAPID_PWR_SHUT_DOWN ((uint8_t)0x05) //!< Disable rapid power shutdown reset type

/**
 * @brief The callback function type for diagnostic session changes.
 * This function is called when the diagnostic session is changed.
 * @param new_sess The new diagnostic session that has been set.
 */
typedef void (*uds_diag_sess_cbk_t)(uint8_t new_sess);

/**
 * @brief The function type for sending data over ISO-TP.
 * This function is used to send data over the ISO-TP protocol.
 * @param handle_ptr Pointer to the UDS handle.
 * @param data_ptr Pointer to the data to be sent.
 * @param data_size Size of the data to be sent. 
 */
typedef void (*uds_iso_tp_send_func_t)(void *handle_ptr, uint8_t *data_ptr, uint16_t data_size);

/**
 * @brief The function type for ECU reset operations.
 * This function is called to perform an ECU reset operation.
 * @param reset_type The type of reset operation to be performed.
 */
typedef void (*uds_ecu_reset_func_t)(uint8_t reset_type);

/**
 * @brief The function type for security access level changes.
 * This function is called when the security level is changed.
 * @param new_level The new security access level that has been set.
 */
typedef void (*uds_sec_acc_cbk_t)(uint8_t new_level);

/**
 * @brief The function type for calculating security access keys.
 * This function is used to calculate the security access key based on the provided seed.
 * @param level The security access level.
 * @param seed_ptr Pointer to the seed data.
 * @param key_ptr Pointer to the key data to be calculated.
 * @param seed_size Size of the seed data.
 * @param key_size Size of the key data.
 */
typedef void (*uds_sec_acc_calc_func_t)(
	uint8_t level,
	uint8_t *seed_ptr,
	uint8_t *key_ptr,
	uint8_t seed_size,
	uint8_t key_size
);

/**
 * @brief The function type for getting the security seed.
 * This function is used to retrieve the security seed for a given security level.
 * @param level The security access level.
 * @param seed_ptr Pointer to the buffer where the seed will be stored.
 * @param seed_size Size of the seed buffer.
 */
typedef void (*uds_sec_acc_get_seed_func_t)(
	uint8_t level,
	uint8_t *seed_ptr,
	uint8_t seed_size
);

/**
 * @brief The function type for a routine identifier (RID) function.
 * This function is called to execute a specific routine identified by the RID.
 * @param arg_rid_ptr Pointer to the argument structure for the RID.
 * This structure contains the current state, ID, arguments, and other necessary information for the routine.
 */
typedef void (*uds_rid_func_ptr_t)(void *arg_rid_ptr);

/**
 * @brief The callback function type for routine download.
 * This function is called when a routine download is requested.
 * @param arg_handle_ptr Pointer to the UDS routine download handle.
 * @return true if the routine download was successful, false otherwise.
 */
typedef bool (*uds_routine_down_cbk_t)(void *arg_handle_ptr);

/**
 * @brief The callback function type for request transfer exit.
 * This function is called when a request transfer exit is requested.
 */
typedef void (*uds_req_transfer_exit_cbk_t)(void);

/**
 * @brief The callback function type for transfer data.
 * This function is called when data transfer is requested.
 * @param arg_handle_ptr Pointer to the UDS transfer data handle.
 */
typedef void (*uds_transfer_data_cbk_t)(void *arg_handle_ptr);

typedef void (*uds_nvm_write_func_t)(uint32_t addr, const uint8_t *data_ptr, uint16_t data_size);
typedef void (*uds_nvm_read_func_t)(uint32_t addr, uint8_t *data_ptr, uint16_t data_size);

typedef enum {
	// user implement this
	UDS_RID_STATE_START,
	UDS_RID_STATE_RUNNING,
	UDS_RID_STATE_DONE,

	// internal states do not use in user code
	UDS_RID_STATE_IDLE,
	UDS_RID_STATE_TIMEOUT
} uds_rid_state_e;

/**
 * @brief Structure representing a diagnostic session.
 * This structure contains the diagnostic session control information,
 * including the session type and the minimum required security level.
 * It is used to manage the diagnostic sessions in the UDS protocol.
 */
typedef struct {
	/** @brief The current diagnostic session. */
	uint8_t diag_sess;
	/** @brief The minimum required security level for this session. */
	uint8_t req_security_level;
} uds_diag_sess_s;

/**
 * @brief Structure representing an ECU reset operation.
 * This structure contains the ECU reset type, the minimum required security level,
 * and a pointer to the allowed diagnostic sessions for this reset operation.
 */
typedef struct {
	uint8_t reset_type; //!< The ECU reset type.
	uint8_t req_security_level; //!< The minimum required security level for this session.
	uint8_t *diag_sess_ptr; //!< Pointer to the allowed diagnostic sessions.
	int8_t num_diag_sess; //!< The number of allowed diagnostic sessions.
} uds_ecu_reset_s;

/**
 * @brief Structure representing a security access level.
 * This structure contains the seed and key levels, pointers to the seed and key data,
 * their sizes, and the allowed diagnostic sessions for this security access.
 */
typedef struct {
	uint8_t seed_level; //!< The security access level for requesting a seed.
	uint8_t key_level; //!< The security access level for sending a key.
	uint8_t *seed_ptr; //!< Pointer to the seed data. Mandatory
	uint8_t *key_ptr; //!< Pointer to the key data. Mandatory
	int8_t seed_size; //!< Size of the seed data. Mandatory
	int8_t key_size; //!< Size of the key data. Mandatory
	uint8_t *diag_sess_ptr; //!< Pointer to the allowed diagnostic sessions. Mandatory
	int8_t num_diag_sess; //!< The number of allowed diagnostic sessions.
} uds_security_access_s;

/**
 * @brief Structure representing a routine identifier (RID).
 * This structure contains the current state of the RID, its ID, pointers to the arguments and results,
 * the required security level, allowed diagnostic sessions, and a function pointer to the RID function.
 */
typedef struct {
	/** @brief Accessible by user in rid function */
	uds_rid_state_e curr_state; //!< current state of the RID
	uint16_t id; //!< unique identifier for the RID, used to identify the routine
	uint8_t *argument_ptr; //!< pointer to the argument buffer, can be NULL if no argument is expected
	uint8_t *result_ptr; //!< pointer to the result buffer, can be NULL if no result is expected


	/** @brief User dont change this values in rid function, after configuration */
	uint8_t argument_size; //!< size of the argument buffer, can be 0 if no argument is expected
	uint8_t result_size; //!< size of the result buffer, can be 0 if no result is expected
	uint8_t req_security_level; //!< minimum required security level for this RID
	uint8_t *diag_sess_ptr; //!< allowed diagnostic sessions for this RID
	int8_t num_diag_sess; //!< number of allowed diagnostic sessions for this RID
	uint64_t run_timeout_ms; //!< timeout for running this RID in milliseconds
	uint64_t start_time_ms; //!< start timestamp of the RID in milliseconds
	uds_rid_func_ptr_t func_ptr; //!< function pointer to the RID function
} uds_rid_s;

/**
 * @brief Structure representing a Data Identifier (DID).
 * This structure contains the DID, a pointer to the argument buffer, the size of the argument,
 * the required security level, allowed diagnostic sessions, and the number of allowed diagnostic sessions.
 */
typedef struct {
	uint16_t did; //!< unique identifier for the DID, used to identify the data
	/**pointer to the buffer, mandatory.
	 * Value received by client is going to be stored here or
	 * Client is going to read from here
	 **/
	uint8_t *buf_ptr;
	uint8_t buf_size; //!< size of the buffer, mandatory
	bool write_access; //!< is it allowed to write to buffer
	uint8_t req_security_level; //!< minimum required security level for this DID
	uint8_t *diag_sess_ptr; //!< pointer to the allowed diagnostic sessions, mandatory
	int8_t num_diag_sess; //!< number of allowed diagnostic sessions
} uds_did_s;

/**
 * @brief Structure representing a routine download request.
 */
typedef struct {
	/** @brief User dont change this values in callback function, after configuration */
	uds_routine_down_cbk_t cbk_ptr; //!< callback function to be called when the routine download is requested
	uint64_t mem_addr; //!< memory address where the routine download will be stored. filled at runtime
	uint64_t mem_size; //!< size of the memory where the routine download will be stored. filled at runtime
	uint8_t mem_addr_len; //!< length of the memory address field. filled at runtime
	uint8_t mem_size_len; //!< length of the memory size field. filled at runtime
	uint16_t block_size; //!< size of each block in the transfer data
	uint8_t req_security_level; //!< minimum required security level for this routine download request
	uint8_t *diag_sess_ptr; //!< pointer to the allowed diagnostic sessions for this routine download request
	int8_t num_diag_sess; //!< number of allowed diagnostic sessions for this routine download request
} uds_routine_download_s;

typedef struct {
	uds_req_transfer_exit_cbk_t cbk_ptr; //!< callback function to be called when the request transfer exit is requested
	uint8_t req_security_level; //!< minimum required security level for this request transfer exit
	uint8_t *diag_sess_ptr; //!< pointer to the allowed diagnostic sessions for this request transfer exit
	int8_t num_diag_sess; //!< number of allowed diagnostic sessions for this request transfer exit
} uds_req_transfer_exit_s;

typedef struct {
	uint8_t *data_ptr; //!< pointer to the data buffer, mandatory
	uint16_t data_size; //!< size of the data buffer, mandatory
	uint16_t recv_size; //!< size of the received data, can be 0 if no data is received yet
	uds_transfer_data_cbk_t cbk_ptr; //!< callback function to be called when the transfer data is requested
	uint8_t req_security_level; //!< minimum required security level for this request transfer exit
	uint8_t *diag_sess_ptr; //!< pointer to the allowed diagnostic sessions for this request transfer exit
	int8_t num_diag_sess; //!< number of allowed diagnostic sessions for this request transfer exit
	uint8_t bsc; //!< block sequence counter
	uint64_t mem_addr; //!< memory address where the transfer data will be stored, filled at runtime
} uds_transfer_data_s;

typedef struct {
	uint8_t high;
	uint8_t mid;
	uint8_t low;
} uds_dtc_id_s;

typedef union {
	uint8_t r;
	struct {
		uint8_t testFailed : 1;
		uint8_t testFailedThisOperationCycle : 1;
		uint8_t pendingDTC : 1;
		uint8_t confirmedDTC : 1;
		uint8_t testNotCompletedSinceLastClear : 1;
		uint8_t testFailedSinceLastClear : 1;
		uint8_t testNotCompletedThisOperationCycle : 1;
	} b;
} uds_dtc_st_u;

typedef struct {
	uds_dtc_st_u status;
	uds_dtc_id_s id; //!< DTC identifier
} uds_dtc_s;

/**
 * @brief Structure representing the enabled UDS services.
 * This structure uses bit fields to indicate which UDS services are enabled.
 */
typedef struct {
	uint32_t diag_sess_ctrl : 1;
	uint32_t tester_present : 1;
	uint32_t ecu_reset : 1;
	uint32_t security_access : 1;
	uint32_t routine_ctrl : 1;
	uint32_t write_data_by_id : 1;
	uint32_t read_data_by_id : 1;
	uint32_t routine_download : 1;
	uint32_t req_transfer_exit : 1;
	uint32_t transfer_data : 1;
	uint32_t read_dtc_info : 1;
} uds_is_serv_en_s; //!< Is service enabled?

/**
 * @brief Configuration structure for UDS.
 * This structure contains all the necessary configuration parameters for the UDS implementation.
 * It includes pointers to buffers, function pointers, diagnostic sessions, ECU reset types,
 * security access types, and other relevant parameters.
 * 
 * This uds implementation fully abstracted from the specific microcontroller or platform,
 * allowing it to be used in various environments with minimal changes.
 * The configuration structure is designed to be flexible and extensible,
 * 
 * Multiple instances of this configuration can be created to support different UDS setups.
 */
typedef struct {
	uds_is_serv_en_s is_serv_en; //!< is service enabled

	uint8_t *rx_ptr; //!< pointer to the receive buffer
	uint8_t rx_buf_size; //!< size of the receive buffer

	uint8_t *tx_ptr; //!< pointer to the transmit buffer
	uint8_t tx_buf_size; //!< size of the transmit buffer

	void *iso_tp_handle_ptr; //!< pointer to the ISO-TP handle
	uds_iso_tp_send_func_t iso_tp_send_func_ptr; //!< function pointer to the ISO-TP send function

	uds_diag_sess_s *avail_diag_sess_ptr; //!< pointer to the available diagnostic sessions
	int8_t num_avail_diag_sess; //!< number of available diagnostic sessions
	uds_diag_sess_cbk_t diag_sess_cbk_ptr; //!< optional callback for session change

	uint16_t p2_server_max; //!< P2 server max time in milliseconds
	uint16_t p2_star_server_max; //!< P2* server max time in 10 milliseconds units

	uds_ecu_reset_func_t ecu_reset_func_ptr; //!< function pointer to the ECU reset function
	uds_ecu_reset_s *ecu_reset_ptr; //!< pointer to the available ECU reset types
	int8_t num_ecu_reset; //!< number of available ECU reset types

	uds_sec_acc_cbk_t sec_acc_cbk_ptr; //!< optional callback for security access change
	uds_security_access_s *sec_acc_ptr; //!< pointer to the available security access types
	int8_t num_sec_acc; //!< number of available security access types
	uds_sec_acc_calc_func_t sec_acc_calc_func_ptr; //!< function pointer to the security access calculation function, mandatory
	uds_sec_acc_get_seed_func_t sec_acc_get_seed_func_ptr; //!< function pointer to the security access get seed function, mandatory

	abs_tim_handle_s *abs_tim_handle_ptr; //!< pointer to the abstract timer handle, mandatory

	uds_rid_s *rid_ptr; //!< optional, available RID list
	int32_t num_rid; //!< number of available RID

	uds_did_s *did_ptr; //!< optional, available DID list
	int32_t num_did; //!< number of available DID

	uds_routine_download_s routine_download; //!< routine download configuration
	uds_req_transfer_exit_s req_transfer_exit; //!< request transfer exit configuration

	uds_transfer_data_s transfer_data; //!< transfer data structure

	uint8_t startup_security_level; //!< Security level at startup
	uint8_t startup_diag_sess; //!< Diagnostic session at startup

	uds_dtc_s *dtc_ptr; //!< pointer to the DTC list, can be NULL if DTCs are not used
	int32_t num_dtc; //!< number of DTCs, can be 0 if DTCs are not used
	/// function pointer for NVM write operations, mandatory if dtc service is enabled
	uds_nvm_write_func_t nvm_write_func_ptr;
	/// function pointer for NVM read operations, mandatory if dtc service is enabled
	uds_nvm_read_func_t nvm_read_func_ptr;

	bool generate_pos_resp_prog;
	bool generate_pos_resp_extd;
} uds_cfg_s;

/**
 * @brief UDS handle structure.
 * This structure contains the state and configuration of the UDS implementation.
 * It is used to manage the UDS communication and services.
 * 
 * Multiple instances of this handle can be created to manage different UDS configurations.
 */
typedef struct {
	bool is_init; //!< true if the UDS handle is initialized
	uds_cfg_s *cfg_ptr; //!< pointer to the UDS configuration

	uint8_t diag_sess; //!< current diagnostic session
	uint8_t diag_sess_prev; //!< previous diagnostic session

	uint8_t recv_packet_size; //!< size of the received packet

	uint8_t security_level; //!< current security access level. id of the requested seed
	uint8_t security_level_prev; //!< previous security access level

	bool is_rid_running; //!< true if any RID is running

	uint8_t bsc; //!< block sequence counter for transfer data
	uint64_t transfer_data_addr; //!< address for transfer data, used in transfer data service

	uds_dtc_st_u dtc_available_st_mask; //!< DTC available status mask
} uds_handle_s;

/**
 * @brief Initialize the UDS handle with the given configuration.
 * This function initializes the UDS handle with the provided configuration.
 * 
 * @param handle_ptr Pointer to the UDS handle to be initialized.
 * @param cfg_ptr Pointer to the UDS configuration structure.
 */
void uds_x_init(uds_handle_s *handle_ptr, uds_cfg_s *cfg_ptr);
/**
 * @brief Get the UDS configuration pointer.
 * This function retrieves the pointer to the UDS configuration structure.
 * 
 * @param handle_ptr Pointer to the UDS handle.
 * @return Pointer to the UDS configuration structure.
 */
uds_cfg_s *uds_x_get_cfg_ptr(uds_handle_s *handle_ptr);
/**
 * @brief Process the UDS communication.
 * This function processes the UDS communication based on the received packet.
 * It handles various UDS services and routines.
 * 
 * @param handle_ptr Pointer to the UDS handle.
 */
void uds_x_handler(uds_handle_s *handle_ptr);
/**
 * @brief Put a packet into the UDS handle for processing.
 * This function puts a packet into the UDS handle for further processing.
 * 
 * @param handle_ptr Pointer to the UDS handle.
 * @param data_ptr Pointer to the data to be put into the handle.
 * @param data_size Size of the data to be put into the handle.
 */
void uds_x_put_packet_in(uds_handle_s *handle_ptr, uint8_t *data_ptr, uint8_t data_size);
/**
 * @brief Get the current diagnostic session from the UDS handle.
 * This function retrieves the current diagnostic session from the UDS handle.
 * 
 * @param handle_ptr Pointer to the UDS handle.
 * @return The current diagnostic session.
 */
uint8_t uds_x_get_diag_sess(uds_handle_s *handle_ptr);
/**
 * @brief Get the current security access level from the UDS handle.
 * This function retrieves the current security access level from the UDS handle.
 * 
 * @param handle_ptr Pointer to the UDS handle.
 * @return The current security access level.
 */
uint8_t uds_x_get_security_level(uds_handle_s *handle_ptr);

void uds_x_set_dtc_st(
	uds_handle_s *handle_ptr,
	int32_t dtc_idx,
	bool triggered
);

/**
 * The functions below are wrappers for uds_x_*
 * to provide a simpler interface for the user.
 * 
 * Mandatory to create the uds handle and configuration before using these functions:
 * uds_cfg_s _uds_cfg;
 * uds_handle_s _uds_handle;
 * 
 * in uds.c:
 *    extern uds_cfg_s _uds_cfg;
 *    extern uds_handle_s _uds_handle;
 */
void uds_init(void);
uds_cfg_s *uds_get_cfg_ptr(void);
void uds_handler(void);
void uds_put_packet_in(uint8_t *data_ptr, uint8_t data_size);
uint8_t uds_get_diag_sess(void);
uint8_t uds_get_security_level(void);
void uds_set_dtc_st(
	int32_t dtc_idx,
	bool triggered
);

extern uds_cfg_s _uds_cfg;
extern uds_handle_s _uds_handle;

#endif // UDS_H
