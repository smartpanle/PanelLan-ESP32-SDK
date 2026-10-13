/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "sdkconfig.h"
#if CONFIG_PANELLAN_TOUCH_FT5X06
#include "esp_lcd_touch_ft5x06.h"
#endif
#if CONFIG_PANELLAN_TOUCH_GT1151
#include "esp_lcd_touch_gt1151.h"
#endif
#if CONFIG_PANELLAN_TOUCH_GT911
#include "esp_lcd_touch_gt911.h"
#endif
#if CONFIG_PANELLAN_TOUCH_CST3240
#include "esp_lcd_touch_cst3240.h"
#endif

#include "panellan_touch.h"

static const char *TAG = "panellan_touch";
static i2c_master_bus_handle_t s_i2c_bus;

static esp_err_t panellan_touch_ensure_i2c(const panellan_touch_config_t *cfg) {
    if (s_i2c_bus) {
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .sda_io_num = cfg->sda,
        .scl_io_num = cfg->scl,
        .i2c_port = cfg->i2c_port,
    };
    return i2c_new_master_bus(&bus_cfg, &s_i2c_bus);
}

#if CONFIG_PANELLAN_TOUCH_FT5X06
static esp_err_t panellan_touch_fix_int_gpio(gpio_num_t intr) {
    if (intr == GPIO_NUM_NC) {
        return ESP_OK;
    }
    gpio_config_t int_pullup = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << intr,
        .pull_up_en = 1,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&int_pullup), TAG, "int gpio_config failed");
    ESP_RETURN_ON_ERROR(gpio_set_intr_type(intr, GPIO_INTR_NEGEDGE), TAG, "int type failed");
    ESP_LOGI(TAG, "Touch INT pull-up + NEGEDGE (GPIO %d)", (int)intr);
    return ESP_OK;
}

#endif

esp_err_t panellan_touch_init(const panellan_touch_config_t *cfg,
                          panellan_display_rotation_t rotation,
                          esp_lcd_touch_handle_t *out_touch) {
    ESP_RETURN_ON_FALSE(cfg && out_touch, ESP_ERR_INVALID_ARG, TAG, "null arg");
    ESP_RETURN_ON_FALSE(cfg->ic != PANELLAN_TOUCH_IC_NONE, ESP_ERR_INVALID_ARG, TAG, "no touch IC");

    const char *name = "unknown";
    switch (cfg->ic) {
    case PANELLAN_TOUCH_IC_FT5X06:
        name = "FT5X06";
        break;
    case PANELLAN_TOUCH_IC_GT1151:
        name = "GT1151";
        break;
    case PANELLAN_TOUCH_IC_GT911:
        name = "GT911";
        break;
    case PANELLAN_TOUCH_IC_CST3240:
        name = "CST3240";
        break;
    default:
        break;
    }
    ESP_LOGI(TAG, "Init touch IC=%s res=%dx%d", name, cfg->x_max, cfg->y_max);

    ESP_RETURN_ON_ERROR(panellan_touch_ensure_i2c(cfg), TAG, "i2c bus failed");

    bool swap_xy = false;
    bool mirror_x = false;
    bool mirror_y = false;
    panellan_touch_get_rotation_flags(cfg->rotation_type, rotation, &swap_xy, &mirror_x, &mirror_y);

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = cfg->x_max,
        .y_max = cfg->y_max,
        .rst_gpio_num = cfg->rst,
        .int_gpio_num = cfg->intr,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = swap_xy,
            .mirror_x = mirror_x,
            .mirror_y = mirror_y,
        },
    };

    esp_lcd_panel_io_handle_t io = NULL;
    esp_err_t ret = ESP_ERR_NOT_SUPPORTED;
    const uint32_t clk = cfg->i2c_clk_hz ? cfg->i2c_clk_hz : 400000;

    switch (cfg->ic) {
#if CONFIG_PANELLAN_TOUCH_FT5X06
    case PANELLAN_TOUCH_IC_FT5X06: {
        esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
        io_cfg.scl_speed_hz = clk;
        ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(s_i2c_bus, &io_cfg, &io), TAG, "ft5x06 io");
        ret = esp_lcd_touch_new_i2c_ft5x06(io, &tp_cfg, out_touch);
        if (ret == ESP_OK && cfg->int_pullup) {
            ESP_RETURN_ON_ERROR(panellan_touch_fix_int_gpio(cfg->intr), TAG, "ft5x06 int");
        }
        break;
    }
#endif
#if CONFIG_PANELLAN_TOUCH_GT1151
    case PANELLAN_TOUCH_IC_GT1151: {
        esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_GT1151_CONFIG();
        io_cfg.scl_speed_hz = clk;
        ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(s_i2c_bus, &io_cfg, &io), TAG, "gt1151 io");
        ret = esp_lcd_touch_new_i2c_gt1151(io, &tp_cfg, out_touch);
        break;
    }
#endif
#if CONFIG_PANELLAN_TOUCH_GT911
    case PANELLAN_TOUCH_IC_GT911: {
        esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
        io_cfg.scl_speed_hz = clk;
        ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(s_i2c_bus, &io_cfg, &io), TAG, "gt911 io");
        ret = esp_lcd_touch_new_i2c_gt911(io, &tp_cfg, out_touch);
        break;
    }
#endif
#if CONFIG_PANELLAN_TOUCH_CST3240
    case PANELLAN_TOUCH_IC_CST3240: {
        esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_CST3240_CONFIG();
        io_cfg.scl_speed_hz = clk;
        ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(s_i2c_bus, &io_cfg, &io), TAG, "cst3240 io");
        ret = esp_lcd_touch_new_i2c_cst3240(io, &tp_cfg, out_touch);
        break;
    }
#endif
    default:
        ret = ESP_ERR_NOT_SUPPORTED;
        break;
    }

    return ret;
}
