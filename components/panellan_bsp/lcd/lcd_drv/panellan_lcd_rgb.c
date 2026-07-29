/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"

#if CONFIG_PANELLAN_LCD_ENABLE_RGB

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"
#include "esp_lv_adapter.h"

#include "panellan_lcd_priv.h"

static const char *TAG = "panellan_lcd_rgb";

static size_t panellan_lcd_calc_bounce(const panellan_lcd_config_t *cfg) {
    const panellan_lcd_rgb_cfg_t *rgb = &cfg->u.rgb;
    switch (rgb->bounce_mode) {
    case PANELLAN_LCD_BOUNCE_NONE:
        return 0;
    case PANELLAN_LCD_BOUNCE_CUSTOM:
        return rgb->bounce_buffer_size_px;
    case PANELLAN_LCD_BOUNCE_AUTO:
    default:
        return (rgb->pclk_hz > 15000000)
                   ? (size_t)(cfg->panel.h_res * (cfg->panel.v_res / 16))
                   : 0;
    }
}

esp_err_t panellan_lcd_init_rgb(const panellan_lcd_config_t *cfg,
                            int tear,
                            panellan_display_rotation_t rotation) {
    const panellan_lcd_rgb_cfg_t *rgb = &cfg->u.rgb;

    if (rgb->pulse_vsync_enable && rgb->vsync != GPIO_NUM_NC) {
        gpio_config_t conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << rgb->vsync,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&conf), TAG, "vsync gpio");
        gpio_set_level(rgb->vsync, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    panellan_lcd_bl_gpio_init(&cfg->panel);

    if (rgb->use_spi9_init) {
        ESP_RETURN_ON_ERROR(
            panellan_lcd_spi9_run(rgb->spi9_mosi, rgb->spi9_clk, rgb->spi9_cs,
                              rgb->spi9_host ? rgb->spi9_host : SPI2_HOST,
                              rgb->spi9_cmds, rgb->spi9_cmd_count),
            TAG, "spi9 init");
    }

    const int num_fbs = esp_lv_adapter_get_required_frame_buffer_count(
        (esp_lv_adapter_tear_avoid_mode_t)tear,
        (esp_lv_adapter_rotation_t)rotation);

    esp_lcd_rgb_panel_config_t panel_conf = {
        .clk_src = rgb->clk_src ? rgb->clk_src : LCD_CLK_SRC_PLL160M,
        .dma_burst_size = 64,
        .data_width = 16,
        .de_gpio_num = rgb->de,
        .pclk_gpio_num = rgb->pclk,
        .vsync_gpio_num = rgb->vsync,
        .hsync_gpio_num = rgb->hsync,
        .disp_gpio_num = rgb->disp,
        .data_gpio_nums = {
            rgb->data[0],  rgb->data[1],  rgb->data[2],  rgb->data[3],
            rgb->data[4],  rgb->data[5],  rgb->data[6],  rgb->data[7],
            rgb->data[8],  rgb->data[9],  rgb->data[10], rgb->data[11],
            rgb->data[12], rgb->data[13], rgb->data[14], rgb->data[15],
        },
        .timings = {
            .pclk_hz = rgb->pclk_hz,
            .h_res = cfg->panel.h_res,
            .v_res = cfg->panel.v_res,
            .hsync_back_porch = rgb->hsync_back_porch,
            .hsync_front_porch = rgb->hsync_front_porch,
            .hsync_pulse_width = rgb->hsync_pulse_width,
            .vsync_back_porch = rgb->vsync_back_porch,
            .vsync_front_porch = rgb->vsync_front_porch,
            .vsync_pulse_width = rgb->vsync_pulse_width,
            .flags.pclk_active_neg = rgb->pclk_active_neg,
        },
        .flags.fb_in_psram = 1,
        .num_fbs = num_fbs,
        .bounce_buffer_size_px = panellan_lcd_calc_bounce(cfg),
    };

    ESP_RETURN_ON_ERROR(esp_lcd_new_rgb_panel(&panel_conf, &g_panellan_lcd_rt.panel), TAG, "rgb panel");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(g_panellan_lcd_rt.panel), TAG, "rgb reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(g_panellan_lcd_rt.panel), TAG, "rgb init");
    g_panellan_lcd_rt.io = NULL;
    return ESP_OK;
}

esp_err_t panellan_lcd_deinit_rgb(void) {
    if (g_panellan_lcd_rt.panel) {
        esp_lcd_panel_disp_on_off(g_panellan_lcd_rt.panel, false);
        esp_lcd_panel_del(g_panellan_lcd_rt.panel);
        g_panellan_lcd_rt.panel = NULL;
    }
    g_panellan_lcd_rt.io = NULL;
    return ESP_OK;
}

#endif /* CONFIG_PANELLAN_LCD_ENABLE_RGB */
