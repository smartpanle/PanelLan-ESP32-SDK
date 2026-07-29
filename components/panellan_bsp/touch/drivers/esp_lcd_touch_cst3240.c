/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"

#include "esp_lcd_touch_cst3240.h"

static const char *TAG = "CST3240";

/* =========================================================================
 * CST3240 Register Map (16-bit addressing, big-endian)
 * ========================================================================= */

/* Touch data register (read 7 bytes for coordinate data) */
#define CST3240_REG_TOUCH_DATA          (0xD000)

/* Chip enable / shutdown exit register (write 0x00 bytes to wake up) */
#define CST3240_REG_CHIP_EN             (0xD109)

/* Clear / acknowledge register (write 0xAB after reading data) */
#define CST3240_REG_CLEAR               (0xD000)
#define CST3240_CLEAR_CMD               (0xAB)

/* Status byte bit flags */
#define CST3240_STATUS_PRESS_MASK       (0x04)  /* bit 2: 1 = press, 0 = release */

/* =========================================================================
 * Forward declarations
 * ========================================================================= */
static esp_err_t esp_lcd_touch_cst3240_read_data(esp_lcd_touch_handle_t tp);
static bool esp_lcd_touch_cst3240_get_xy(esp_lcd_touch_handle_t tp,
                                          uint16_t *x, uint16_t *y,
                                          uint16_t *strength,
                                          uint8_t *point_num,
                                          uint8_t max_point_num);
static esp_err_t esp_lcd_touch_cst3240_del(esp_lcd_touch_handle_t tp);

/* I2C read/write helpers (16-bit register addressing) */
static esp_err_t cst3240_i2c_write(esp_lcd_touch_handle_t tp, uint16_t reg,
                                    uint8_t data);
static esp_err_t cst3240_i2c_write_no_data(esp_lcd_touch_handle_t tp,
                                            uint16_t reg);
static esp_err_t cst3240_i2c_read(esp_lcd_touch_handle_t tp, uint16_t reg,
                                   uint8_t *data, uint8_t len);

/* Init */
static esp_err_t cst3240_init(esp_lcd_touch_handle_t tp);

/* =========================================================================
 * Public API: Constructor
 * ========================================================================= */

esp_err_t esp_lcd_touch_new_i2c_cst3240(const esp_lcd_panel_io_handle_t io,
                                         const esp_lcd_touch_config_t *config,
                                         esp_lcd_touch_handle_t *out_touch) {
    esp_err_t ret = ESP_OK;

    assert(config != NULL);
    assert(out_touch != NULL);

    /* Allocate main structure */
    esp_lcd_touch_handle_t cst3240 = heap_caps_calloc(1, sizeof(esp_lcd_touch_t),
                                                       MALLOC_CAP_DEFAULT);
    ESP_GOTO_ON_FALSE(cst3240, ESP_ERR_NO_MEM, err, TAG,
                      "no mem for CST3240 controller");

    /* Communication interface */
    cst3240->io = io;

    /* Only supported callbacks are set */
    cst3240->read_data = esp_lcd_touch_cst3240_read_data;
    cst3240->get_xy    = esp_lcd_touch_cst3240_get_xy;
    cst3240->del       = esp_lcd_touch_cst3240_del;

    /* Mutex */
    cst3240->data.lock.owner = portMUX_FREE_VAL;

    /* Save config */
    memcpy(&cst3240->config, config, sizeof(esp_lcd_touch_config_t));

    /*
     * Prepare pin for touch interrupt.
     * CST3240 INT pin is open-drain — internal pull-up is required,
     * otherwise the line floats and the ESP32 never sees the interrupt.
     */
    if (cst3240->config.int_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t int_gpio_config = {
            .mode         = GPIO_MODE_INPUT,
            .pin_bit_mask = BIT64(cst3240->config.int_gpio_num),
            .pull_up_en   = 1,
        };
        ret = gpio_config(&int_gpio_config);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "GPIO config failed");
    }

    /* Prepare pin for touch controller reset */
    if (cst3240->config.rst_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t rst_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = BIT64(cst3240->config.rst_gpio_num)
        };
        ret = gpio_config(&rst_gpio_config);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "GPIO config failed");
    }

    /* Init controller (reset sequence + wake-up command) */
    ret = cst3240_init(cst3240);
    ESP_GOTO_ON_ERROR(ret, err, TAG, "CST3240 init failed");

err:
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error (0x%x)! Touch controller CST3240 init failed!", ret);
        if (cst3240) {
            esp_lcd_touch_cst3240_del(cst3240);
        }
    }

    *out_touch = cst3240;

    return ret;
}

/* =========================================================================
 * Public API: Read Data
 * ========================================================================= */

static esp_err_t esp_lcd_touch_cst3240_read_data(esp_lcd_touch_handle_t tp) {
    esp_err_t err;
    uint8_t data[7] = {0};

    assert(tp != NULL);

    /* Read 7 bytes from register 0xD000 */
    err = cst3240_i2c_read(tp, CST3240_REG_TOUCH_DATA, data, 7);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "I2C read error: 0x%x", err);
        return err;
    }

    taskENTER_CRITICAL(&tp->data.lock);

    /* Parse touch event from status byte: bit 2 (0x04) = press */
    if (data[0] & CST3240_STATUS_PRESS_MASK) {
        tp->data.points = 1;

        /*
         * CST3240 coordinate format (12-bit resolution):
         *   data[1]    = X[11:4]  (high 8 bits of X)
         *   data[2]    = Y[11:4]  (high 8 bits of Y)
         *   data[3]    = X[3:0] << 4 | Y[3:0]
         *   data[4..6] = reserved / unused
         */
        tp->data.coords[0].x = ((uint16_t)data[1] << 4)
                             | ((data[3] >> 4) & 0x0F);
        tp->data.coords[0].y = ((uint16_t)data[2] << 4)
                             | (data[3] & 0x0F);
        tp->data.coords[0].strength = 0;  /* CST3240 does not report pressure */
        tp->data.coords[0].track_id = 0;
    } else {
        tp->data.points = 0;
    }

    taskEXIT_CRITICAL(&tp->data.lock);

    /*
     * Write 0xAB to register 0xD000 to clear the interrupt flag.
     * This is required by CST3240 to prepare for the next read cycle.
     * (Matching BSP reference: i2c_write_byte(cst3240_device, 0xD000, 0xAB))
     */
    err = cst3240_i2c_write(tp, CST3240_REG_CLEAR, CST3240_CLEAR_CMD);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send clear command: 0x%x", err);
    }

    return ESP_OK;
}

/* =========================================================================
 * Public API: Get XY
 * ========================================================================= */

static bool esp_lcd_touch_cst3240_get_xy(esp_lcd_touch_handle_t tp,
                                          uint16_t *x, uint16_t *y,
                                          uint16_t *strength,
                                          uint8_t *point_num,
                                          uint8_t max_point_num) {
    assert(tp != NULL);
    assert(x != NULL);
    assert(y != NULL);
    assert(point_num != NULL);
    assert(max_point_num > 0);

    taskENTER_CRITICAL(&tp->data.lock);

    /* CST3240 only reports 1 touch point */
    *point_num = (tp->data.points > max_point_num ? max_point_num : tp->data.points);

    for (size_t i = 0; i < *point_num; i++) {
        x[i] = tp->data.coords[i].x;
        y[i] = tp->data.coords[i].y;

        if (strength) {
            strength[i] = tp->data.coords[i].strength;
        }
    }

    /* Invalidate */
    tp->data.points = 0;

    taskEXIT_CRITICAL(&tp->data.lock);

    return (*point_num > 0);
}

/* =========================================================================
 * Public API: Delete
 * ========================================================================= */

static esp_err_t esp_lcd_touch_cst3240_del(esp_lcd_touch_handle_t tp) {
    assert(tp != NULL);

    /* Reset GPIO pin settings */
    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.int_gpio_num);
    }

    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.rst_gpio_num);
    }

    free(tp);

    return ESP_OK;
}

/* =========================================================================
 * Private: Init (reset sequence + chip wake-up)
 *
 * Matching BSP reference cst3240_init():
 *   1. RST low 10ms → high
 *   2. Wait 100ms for internal stabilization
 *   3. Write register 0xD109 (chip enable, no data bytes)
 *   4. Wait 10ms
 * ========================================================================= */

static esp_err_t cst3240_init(esp_lcd_touch_handle_t tp) {
    esp_err_t ret = ESP_OK;

    /* Hardware reset sequence */
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        ESP_LOGD(TAG, "Resetting CST3240 (RST=GPIO%d)", tp->config.rst_gpio_num);
        gpio_set_level(tp->config.rst_gpio_num,
                       tp->config.levels.reset ? 1 : 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(tp->config.rst_gpio_num,
                       tp->config.levels.reset ? 0 : 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        ESP_LOGD(TAG, "CST3240 reset complete");
    }

    /*
     * Wake up the chip by writing to register 0xD109.
     * The BSP sends this as a write with NO data bytes —
     * just the register address alone acts as the wake-up command.
     */
    ret = cst3240_i2c_write_no_data(tp, CST3240_REG_CHIP_EN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "CST3240 wake-up failed (reg 0x%04X)", CST3240_REG_CHIP_EN);
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    /*
     * CST3240 INT pin is open-drain (same as FT5x06).
     * esp_lcd_touch_register_interrupt_callback() calls gpio_intr_enable()
     * but does NOT set the interrupt type — we must configure it here.
     * CST3240 INT is active-low → use falling edge (NEGEDGE).
     */
    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        gpio_set_intr_type(tp->config.int_gpio_num, GPIO_INTR_NEGEDGE);
        ESP_LOGD(TAG, "CST3240 INT NEGEDGE configured (GPIO %d)",
                 tp->config.int_gpio_num);
    }

    return ret;
}

/* =========================================================================
 * Private: I2C Helpers (16-bit register addressing)
 *
 * CST3240 uses 16-bit big-endian register addresses.
 * The I2C panel IO is configured with lcd_cmd_bits=16,
 * so esp_lcd_panel_io_tx_param / _rx_param send the full 16-bit address.
 * ========================================================================= */

/**
 * @brief Write a register address + 0 data bytes (used for wake-up command)
 */
static esp_err_t cst3240_i2c_write_no_data(esp_lcd_touch_handle_t tp,
                                            uint16_t reg) {
    assert(tp != NULL);

    return esp_lcd_panel_io_tx_param(tp->io, reg, NULL, 0);
}

/**
 * @brief Write a register address + 1 data byte
 */
static esp_err_t cst3240_i2c_write(esp_lcd_touch_handle_t tp,
                                    uint16_t reg, uint8_t data) {
    assert(tp != NULL);

    return esp_lcd_panel_io_tx_param(tp->io, reg, (const uint8_t[]){data}, 1);
}

/**
 * @brief Read from a register address
 */
static esp_err_t cst3240_i2c_read(esp_lcd_touch_handle_t tp,
                                   uint16_t reg, uint8_t *data, uint8_t len) {
    assert(tp != NULL);
    assert(data != NULL);

    return esp_lcd_panel_io_rx_param(tp->io, reg, data, len);
}