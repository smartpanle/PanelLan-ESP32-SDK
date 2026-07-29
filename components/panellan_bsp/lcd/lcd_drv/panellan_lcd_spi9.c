/*
 * SPDX-License-Identifier: Apache-2.0
 * 3-wire 9-bit SPI init helper for RGB panels (ST7701 etc.)
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_log.h"

#include "panellan_lcd.h"

static const char *TAG = "panellan_lcd_spi9";

static void lcd_spi_send_9bit(spi_device_handle_t spi, uint8_t is_data, uint8_t value) {
    uint16_t cmd_field = ((uint16_t)is_data << 8) | value;
    spi_transaction_ext_t trans = (spi_transaction_ext_t){
        .base = {
            .flags = SPI_TRANS_VARIABLE_CMD,
            .cmd = cmd_field,
        },
        .command_bits = 9,
    };
    ESP_ERROR_CHECK(spi_device_transmit(spi, (spi_transaction_t *)&trans));
}

esp_err_t panellan_lcd_spi9_run(gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs,
                            spi_host_device_t host,
                            const panellan_lcd_spi9_cmd_t *cmds, size_t count) {
    ESP_RETURN_ON_FALSE(cmds && count, ESP_ERR_INVALID_ARG, TAG, "no cmds");

    spi_bus_config_t buscfg = {
        .sclk_io_num = clk,
        .mosi_io_num = mosi,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 10 * 1024,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO), TAG, "spi bus");

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_MASTER_FREQ_10M,
        .mode = 0,
        .spics_io_num = cs,
        .queue_size = 7,
    };

    spi_device_handle_t spi = NULL;
    ESP_RETURN_ON_ERROR(spi_bus_add_device(host, &devcfg, &spi), TAG, "spi add");

    for (size_t i = 0; i < count; ++i) {
        const panellan_lcd_spi9_cmd_t *cmd = &cmds[i];
        if (cmd->cmd == 0x00 && cmd->data_bytes == 0 && cmd->delay_ms == 0) {
            break;
        }
        lcd_spi_send_9bit(spi, 0, cmd->cmd);
        if (cmd->data_bytes > 0 && cmd->data != NULL) {
            for (uint8_t j = 0; j < cmd->data_bytes; j++) {
                lcd_spi_send_9bit(spi, 1, cmd->data[j]);
            }
        }
        if (cmd->delay_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(cmd->delay_ms));
        }
    }

    spi_bus_remove_device(spi);
    spi_bus_free(host);
    ESP_LOGI(TAG, "SPI9 init done (%u cmds)", (unsigned)count);
    return ESP_OK;
}
