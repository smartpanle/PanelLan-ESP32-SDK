/*
 * PanelLan touch L2 — wraps Espressif esp_lcd_touch_* drivers.
 * Board (L3) only fills panellan_touch_config_t (pins / IC / rotation policy).
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_lcd_touch.h"
#include "sdkconfig.h"

#include "panellan_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Touch IC selection (set by board config; Kconfig gates which drivers build)
 */
typedef enum {
    PANELLAN_TOUCH_IC_NONE = 0,             /*!< No touch */
    PANELLAN_TOUCH_IC_FT5X06,               /*!< FocalTech FT5x06 family */
    PANELLAN_TOUCH_IC_GT911,                /*!< Goodix GT911 */
    PANELLAN_TOUCH_IC_CST3240,              /*!< Hynitron CST3240 (local driver) */
    PANELLAN_TOUCH_IC_GT1151,               /*!< GT1151 */
} panellan_touch_ic_t;

/**
 * @brief Mapping policy for touch coordinates vs display rotation
 *
 * Touch IC orientation may not match the panel on every bus/module;
 * pick the matching swap/mirror rules via this field
 * (see panellan_touch_get_rotation_flags).
 */
typedef enum {
    PANELLAN_TOUCH_ROTATION_STANDARD = 0,   /*!< Default mapping (RGB / SPI / etc.) */
    PANELLAN_TOUCH_ROTATION_MIPI_DSI,       /*!< Common mapping for MIPI DSI modules */
    PANELLAN_TOUCH_ROTATION_I80,            /*!< Common mapping for I80 (8080) modules */
} panellan_touch_rotation_type_t;

/**
 * @brief Touch init config (filled by board tables)
 */
typedef struct {
    panellan_touch_ic_t ic;                 /*!< Touch IC */
    int x_max;                          /*!< Touch X range (usually panel width) */
    int y_max;                          /*!< Touch Y range (usually panel height) */
    gpio_num_t sda;                     /*!< I2C SDA */
    gpio_num_t scl;                     /*!< I2C SCL */
    gpio_num_t rst;                     /*!< Reset pin; GPIO_NUM_NC if unused */
    gpio_num_t intr;                    /*!< Interrupt pin; GPIO_NUM_NC if unused */
    i2c_port_t i2c_port;                /*!< I2C port number */
    uint32_t i2c_clk_hz;                /*!< I2C clock (Hz); typically 400000 */
    panellan_touch_rotation_type_t rotation_type; /*!< Coordinate rotation mapping policy */
    /** FT5x06 open-drain INT often needs internal pull-up + falling edge; when true, L2 configures GPIO */
    bool int_pullup;
} panellan_touch_config_t;

/**
 * @brief Touch config defaults (no IC, pins NC, I2C0 @ 400 kHz)
 */
#define PANELLAN_TOUCH_CONFIG_DEFAULT()             \
    {                                           \
        .ic = PANELLAN_TOUCH_IC_NONE,               \
        .x_max = 0,                             \
        .y_max = 0,                             \
        .sda = GPIO_NUM_NC,                     \
        .scl = GPIO_NUM_NC,                     \
        .rst = GPIO_NUM_NC,                     \
        .intr = GPIO_NUM_NC,                    \
        .i2c_port = I2C_NUM_0,                  \
        .i2c_clk_hz = 400000,                   \
        .rotation_type = PANELLAN_TOUCH_ROTATION_STANDARD, \
        .int_pullup = false,                    \
    }

/**
 * @brief Create I2C bus from config and init the touch IC
 *
 * @param[in]  cfg       Board touch config
 * @param[in]  rotation  Current display rotation (keep in sync with LCD)
 * @param[out] out_touch esp_lcd_touch handle on success
 * @return
 *      - ESP_OK: success
 *      - ESP_ERR_INVALID_ARG: bad args or ic is NONE
 *      - other: I2C / driver create failure
 */
esp_err_t panellan_touch_init(const panellan_touch_config_t *cfg,
                          panellan_display_rotation_t rotation,
                          esp_lcd_touch_handle_t *out_touch);

/**
 * @brief Map rotation_type + display rotation to esp_lcd_touch swap/mirror flags
 *
 * @param[in]  type      Board rotation policy
 * @param[in]  rotation  Display rotation angle
 * @param[out] swap_xy   Whether to swap XY
 * @param[out] mirror_x  Whether to mirror X
 * @param[out] mirror_y  Whether to mirror Y
 */
void panellan_touch_get_rotation_flags(panellan_touch_rotation_type_t type,
                                   panellan_display_rotation_t rotation,
                                   bool *swap_xy, bool *mirror_x, bool *mirror_y);

#ifdef __cplusplus
}
#endif
