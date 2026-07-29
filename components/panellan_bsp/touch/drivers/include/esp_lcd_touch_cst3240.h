/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new CST3240 touch driver
 *
 * @note  The I2C communication should be initialized before use this function.
 *        CST3240 uses 16-bit big-endian register addressing (unlike FT5x06's 8-bit).
 *
 * @param[in]  io        LCD/Touch panel IO handle (I2C)
 * @param[in]  config    Touch configuration
 * @param[out] out_touch Touch instance handle
 * @return
 *      - ESP_OK                    on success
 *      - ESP_ERR_NO_MEM            if there is no memory for allocating main structure
 *      - ESP_ERR_INVALID_ARG       if parameter is invalid
 */
esp_err_t esp_lcd_touch_new_i2c_cst3240(const esp_lcd_panel_io_handle_t io,
                                         const esp_lcd_touch_config_t *config,
                                         esp_lcd_touch_handle_t *out_touch);

/**
 * @brief I2C address of the CST3240 controller
 *
 * @note  The BSP reference uses 0x5A as the 7-bit I2C address.
 */
#define ESP_LCD_TOUCH_IO_I2C_CST3240_ADDRESS (0x5A)

/**
 * @brief Touch IO configuration structure for CST3240
 *
 * @note  Key differences from FT5x06:
 *        - 16-bit register addressing (control_phase_bytes=2, lcd_cmd_bits=16)
 *        - I2C address 0x5A
 *        - disable_control_phase=1 (no DC pin, pure I2C)
 */
#define ESP_LCD_TOUCH_IO_I2C_CST3240_CONFIG()           \
    {                                                   \
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_CST3240_ADDRESS, \
        .control_phase_bytes = 2,                       \
        .dc_bit_offset = 0,                             \
        .lcd_cmd_bits = 16,                             \
        .flags =                                        \
        {                                               \
            .disable_control_phase = 1,                 \
        }                                               \
    }

#ifdef __cplusplus
}
#endif
