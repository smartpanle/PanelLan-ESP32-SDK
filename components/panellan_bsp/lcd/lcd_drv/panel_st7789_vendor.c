/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * ST7789 vendor init helper.
 *
 * Unlike st7796/nv3041, IDF's esp_lcd_new_panel_st7789() does not auto-send
 * a glass init table via vendor_config.init_cmds. This file provides
 * panel_st7789_init(): after new_panel, send the board-supplied
 * st7789_lcd_init_cmd_t[] one-by-one over panel_io (including delays).
 *
 * Typical callers: I80 / SPI paths after esp_lcd_new_panel_st7789()
 * (e.g. ZX2D80). Init-table data itself still comes from board panellan_bsp_config.
 */

#include <stdlib.h>
#include <sys/cdefs.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_lcd_panel_st7789.h"

#include "st7789_vendor.h"

static const char *TAG = "st7789_vendor";

esp_err_t panel_st7789_init(esp_lcd_panel_io_handle_t io, const st7789_vendor_config_t *vendor_config) {
    ESP_RETURN_ON_FALSE(io, ESP_ERR_INVALID_ARG, TAG, "invalid io handle");

    // LCD goes into sleep mode and display will be turned off after power on
    // reset, exit sleep mode first
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_SLPOUT, NULL, 0),
                        TAG, "send SLPOUT failed");
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL,
                                                  (uint8_t[]){0x00}, 1),
                        TAG, "send MADCTL failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_COLMOD,
                                                  (uint8_t[]){0x55}, 1),
                        TAG, "send COLMOD failed");

    // Select init commands: use vendor_config if provided, otherwise use defaults
    const st7789_lcd_init_cmd_t *init_cmds = NULL;
    uint16_t init_cmds_size = 0;
    if (vendor_config && vendor_config->init_cmds) {
        init_cmds = vendor_config->init_cmds;
        init_cmds_size = vendor_config->init_cmds_size;
    } else {
        ESP_LOGE(TAG, "vendor_config == NULL");
        assert(0);
        return ESP_FAIL;
    }

    // Check if MADCTL or COLMOD will be overwritten by the init sequence
    for (int i = 0; i < init_cmds_size; i++) {
        switch (init_cmds[i].cmd) {
        case LCD_CMD_MADCTL:
            ESP_LOGW(TAG,
                     "The %02Xh command has been used and will be overwritten "
                     "by external initialization sequence",
                     init_cmds[i].cmd);
            break;
        case LCD_CMD_COLMOD:
            ESP_LOGW(TAG,
                     "The %02Xh command has been used and will be overwritten "
                     "by external initialization sequence",
                     init_cmds[i].cmd);
            break;
        default:
            break;
        }
    }

    // Send all init commands
    for (int i = 0; i < init_cmds_size; i++) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, init_cmds[i].cmd,
                                                      init_cmds[i].data,
                                                      init_cmds[i].data_bytes),
                            TAG, "send init command failed");
        vTaskDelay(pdMS_TO_TICKS(init_cmds[i].delay_ms));
    }
    ESP_LOGD(TAG, "send init commands success");

    return ESP_OK;
}
