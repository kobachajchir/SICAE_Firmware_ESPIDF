/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "sdkconfig.h"
#include "hal/adc_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Driver Backgrounds
 *
 * --------------------------------------------------------------------------------------------------------
 * |                              Conversion Frame                                                        |
 * --------------------------------------------------------------------------------------------------------
 * | Conversion Result | Conversion Result | Conversion Result | Conversion Result |  ...                 |
 * --------------------------------------------------------------------------------------------------------
 *
 * ADC continuous mode conversion is made up with multiple Conversion Frames.
 * - Conversion Frame:      One Conversion Frame contains multiple Conversion Results.
 *                          Conversion Frame size is configured in `adc_continuous_handle_cfg_t:conv_frame_size`, in bytes.
 *                          Each time driver see an interrupt event, this means one Conversion Frame is generated by the hardware.
 * - Conversion Result:     One Conversion Result contains multiple bytes (see `SOC_ADC_DIGI_RESULT_BYTES`). Its
 *                          structure is `adc_digi_output_data_t`, including ADC Unit, ADC Channel and Raw Data.
 *
 * For example:
 * conv_frame_size = 100,
 * then one Conversion Frame contains (100 / `SOC_ADC_DIGI_RESULT_BYTES`) pieces of Conversion Results
 */

/**
 * @brief ADC read max timeout value, it may make the ``adc_continuous_read`` block forever if the OS supports
 */
#define ADC_MAX_DELAY UINT32_MAX

/**
 * @brief Type of adc continuous mode driver handle
 */
typedef struct adc_continuous_ctx_t *adc_continuous_handle_t;

/**
 * @brief ADC continuous mode driver initial configurations
 */
typedef struct {
    uint32_t max_store_buf_size;    ///< Max length of the conversion Results that driver can store, in bytes.
    uint32_t conv_frame_size;       ///< Conversion frame size, in bytes. This should be in multiples of `SOC_ADC_DIGI_DATA_BYTES_PER_CONV`.
} adc_continuous_handle_cfg_t;

/**
 * @brief ADC continuous mode driver configurations
 */
typedef struct {
    uint32_t pattern_num;                   ///< Number of ADC channels that will be used
    adc_digi_pattern_config_t *adc_pattern; ///< List of configs for each ADC channel that will be used
    uint32_t sample_freq_hz;                /*!< The expected ADC sampling frequency in Hz. Please refer to `soc/soc_caps.h` to know available sampling frequency range*/
    adc_digi_convert_mode_t conv_mode;      ///< ADC DMA conversion mode, see `adc_digi_convert_mode_t`.
    adc_digi_output_format_t format;        ///< ADC DMA conversion output format, see `adc_digi_output_format_t`.
} adc_continuous_config_t;

/**
 * @brief Event data structure
 * @note The `conv_frame_buffer` is maintained by the driver itself, so never free this piece of memory.
 */
typedef struct {
    uint8_t *conv_frame_buffer;             ///< Pointer to conversion result buffer for one conversion frame
    uint32_t size;                          ///< Conversion frame size
} adc_continuous_evt_data_t;

/**
 * @brief Prototype of ADC continuous mode event callback
 *
 * @param[in] handle    ADC continuous mode driver handle
 * @param[in] edata     Pointer to ADC contunuous mode event data
 * @param[in] user_data User registered context, registered when in `adc_continuous_register_event_callbacks()`
 *
 * @return Whether a high priority task is woken up by this function
 */
typedef bool (*adc_continuous_callback_t)(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data);

/**
 * @brief Group of ADC continuous mode callbacks
 *
 * @note These callbacks are all running in an ISR environment.
 * @note When CONFIG_ADC_CONTINUOUS_ISR_IRAM_SAFE is enabled, the callback itself and functions called by it should be placed in IRAM.
 *       Involved variables should be in internal RAM as well.
 */
typedef struct {
    adc_continuous_callback_t on_conv_done;    ///< Event callback, invoked when one conversion frame is done. See `@brief Driver Backgrounds` to konw `conversion frame` concept.
    adc_continuous_callback_t on_pool_ovf;     ///< Event callback, invoked when the internal pool is full.
} adc_continuous_evt_cbs_t;

/**
 * @brief Initialize ADC continuous driver and get a handle to it
 *
 * @param[in]  hdl_config  Pointer to ADC initilization config. Refer to ``adc_continuous_handle_cfg_t``.
 * @param[out] ret_handle  ADC continuous mode driver handle
 *
 * @return
 *         - ESP_ERR_INVALID_ARG   If the combination of arguments is invalid.
 *         - ESP_ERR_NOT_FOUND     No free interrupt found with the specified flags
 *         - ESP_ERR_NO_MEM        If out of memory
 *         - ESP_OK                On success
 */
esp_err_t adc_continuous_new_handle(const adc_continuous_handle_cfg_t *hdl_config, adc_continuous_handle_t *ret_handle);

/**
 * @brief Set ADC continuous mode required configurations
 *
 * @param[in] handle ADC continuous mode driver handle
 * @param[in] config Refer to ``adc_digi_config_t``.
 *
 * @return
 *      - ESP_ERR_INVALID_STATE: Driver state is invalid, you shouldn't call this API at this moment
 *      - ESP_ERR_INVALID_ARG:   If the combination of arguments is invalid.
 *      - ESP_OK:                On success
 */
esp_err_t adc_continuous_config(adc_continuous_handle_t handle, const adc_continuous_config_t *config);

/**
 * @brief Register callbacks
 *
 * @note User can deregister a previously registered callback by calling this function and setting the to-be-deregistered callback member in
 *       the `cbs` structure to NULL.
 * @note When CONFIG_ADC_CONTINUOUS_ISR_IRAM_SAFE is enabled, the callback itself and functions called by it should be placed in IRAM.
 *       Involved variables (including `user_data`) should be in internal RAM as well.
 * @note You should only call this API when the ADC continuous mode driver isn't started. Check return value to know this.
 *
 * @param[in] handle    ADC continuous mode driver handle
 * @param[in] cbs       Group of callback functions
 * @param[in] user_data User data, which will be delivered to the callback functions directly
 *
 * @return
 *        - ESP_OK:                On success
 *        - ESP_ERR_INVALID_ARG:   Invalid arguments
 *        - ESP_ERR_INVALID_STATE: Driver state is invalid, you shouldn't call this API at this moment
 */
esp_err_t adc_continuous_register_event_callbacks(adc_continuous_handle_t handle, const adc_continuous_evt_cbs_t *cbs, void *user_data);

/**
 * @brief Start the ADC under continuous mode. After this, the hardware starts working.
 *
 * @param[in]  handle              ADC continuous mode driver handle
 *
 * @return
 *         - ESP_ERR_INVALID_STATE Driver state is invalid.
 *         - ESP_OK                On success
 */
esp_err_t adc_continuous_start(adc_continuous_handle_t handle);

/**
 * @brief Read bytes from ADC under continuous mode.
 *
 * @param[in]  handle              ADC continuous mode driver handle
 * @param[out] buf                 Conversion result buffer to read from ADC. Suggest convert to `adc_digi_output_data_t` for `ADC Conversion Results`.
 *                                 See `@brief Driver Backgrounds` to know this concept.
 * @param[in]  length_max          Expected length of the Conversion Results read from the ADC, in bytes.
 * @param[out] out_length          Real length of the Conversion Results read from the ADC via this API, in bytes.
 * @param[in]  timeout_ms          Time to wait for data via this API, in millisecond.
 *
 * @return
 *         - ESP_ERR_INVALID_STATE Driver state is invalid. Usually it means the ADC sampling rate is faster than the task processing rate.
 *         - ESP_ERR_TIMEOUT       Operation timed out
 *         - ESP_OK                On success
 */
esp_err_t adc_continuous_read(adc_continuous_handle_t handle, uint8_t *buf, uint32_t length_max, uint32_t *out_length, uint32_t timeout_ms);

/**
 * @brief Stop the ADC. After this, the hardware stops working.
 *
 * @param[in]  handle              ADC continuous mode driver handle
 *
 * @return
 *         - ESP_ERR_INVALID_STATE Driver state is invalid.
 *         - ESP_OK                On success
 */
esp_err_t adc_continuous_stop(adc_continuous_handle_t handle);

/**
 * @brief Deinitialize the ADC continuous driver.
 *
 * @param[in]  handle              ADC continuous mode driver handle
 *
 * @return
 *         - ESP_ERR_INVALID_STATE Driver state is invalid.
 *         - ESP_OK                On success
 */
esp_err_t adc_continuous_deinit(adc_continuous_handle_t handle);

/**
 * @brief Get ADC channel from the given GPIO number
 *
 * @param[in]  io_num     GPIO number
 * @param[out] unit_id    ADC unit
 * @param[out] channel    ADC channel
 *
 * @return
 *        - ESP_OK:              On success
 *        - ESP_ERR_INVALID_ARG: Invalid argument
 *        - ESP_ERR_NOT_FOUND:   The IO is not a valid ADC pad
 */
esp_err_t adc_continuous_io_to_channel(int io_num, adc_unit_t *unit_id, adc_channel_t *channel);

/**
 * @brief Get GPIO number from the given ADC channel
 *
 * @param[in]  unit_id    ADC unit
 * @param[in]  channel    ADC channel
 * @param[out] io_num     GPIO number
 *
 * @param
 *       - ESP_OK:              On success
 *       - ESP_ERR_INVALID_ARG: Invalid argument
 */
esp_err_t adc_continuous_channel_to_io(adc_unit_t unit_id, adc_channel_t channel, int *io_num);


#ifdef __cplusplus
}
#endif
