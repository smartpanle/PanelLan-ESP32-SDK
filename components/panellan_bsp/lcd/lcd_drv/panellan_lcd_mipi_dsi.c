/*
 * SPDX-License-Identifier: Apache-2.0
 * MIPI DSI LCD — mirrors tmp_sdk lcd_init_mipi_dsi.c
 */

#include "sdkconfig.h"

#if CONFIG_PANELLAN_LCD_ENABLE_MIPI_DSI

#include "esp_check.h"
#include "esp_log.h"
#include "esp_idf_version.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "esp_lv_adapter.h"
#if CONFIG_PANELLAN_LCD_PANEL_ST7701
#include "esp_lcd_st7701.h"
#endif

#include "panellan_lcd_priv.h"

static const char *TAG = "panellan_lcd_mipi";

static esp_err_t panellan_lcd_mipi_ldo_on(const panellan_lcd_mipi_cfg_t *mipi) {
    if (mipi->ldo_chan_id < 0) {
        return ESP_OK;
    }
    esp_ldo_channel_handle_t ldo = NULL;
    esp_ldo_channel_config_t conf = {
        .chan_id = mipi->ldo_chan_id,
        .voltage_mv = mipi->ldo_voltage_mv ? mipi->ldo_voltage_mv : 2700,
    };
    return esp_ldo_acquire_channel(&conf, &ldo);
}

esp_err_t panellan_lcd_init_mipi_dsi(const panellan_lcd_config_t *cfg,
                                 int tear,
                                 panellan_display_rotation_t rotation) {
    const panellan_lcd_mipi_cfg_t *mipi = &cfg->u.mipi;
    const panellan_lcd_panel_t *panel = &cfg->panel;

    ESP_RETURN_ON_ERROR(panellan_lcd_mipi_ldo_on(mipi), TAG, "ldo");

    if (mipi->pwr_gpio != GPIO_NUM_NC) {
        gpio_config_t pwr = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << mipi->pwr_gpio,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&pwr), TAG, "pwr gpio");
        gpio_set_level(mipi->pwr_gpio, mipi->pwr_on_level);
    }

    panellan_lcd_bl_gpio_init(panel);

    esp_lcd_dsi_bus_config_t bus_config = {
        .bus_id = 0,
        .num_data_lanes = mipi->lane_num ? mipi->lane_num : 2,
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = mipi->lane_bit_rate_mbps ? mipi->lane_bit_rate_mbps : 1000,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_dsi_bus(&bus_config, &g_panellan_lcd_rt.dsi_bus), TAG, "dsi bus");

    esp_lcd_dbi_io_config_t dbi_config = {
        .virtual_channel = 0,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_dbi(g_panellan_lcd_rt.dsi_bus, &dbi_config, &g_panellan_lcd_rt.io),
                        TAG, "dbi io");

#if CONFIG_PANELLAN_LCD_PANEL_ST7701
    if (panel->panel_ic != PANELLAN_LCD_PANEL_ST7701) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    const int num_fbs = esp_lv_adapter_get_required_frame_buffer_count(
        (esp_lv_adapter_tear_avoid_mode_t)tear,
        (esp_lv_adapter_rotation_t)rotation);

    static esp_lcd_dpi_panel_config_t dpi_config;
    dpi_config = (esp_lcd_dpi_panel_config_t){
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = mipi->dpi_clock_freq_mhz ? mipi->dpi_clock_freq_mhz : 28,
        .virtual_channel = 0,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
        .in_color_format = (panel->bits_per_pixel == 24) ? LCD_COLOR_FMT_RGB888 : LCD_COLOR_FMT_RGB565,
#else
        .pixel_format = (panel->bits_per_pixel == 24)
                            ? LCD_COLOR_PIXEL_FORMAT_RGB888
                            : LCD_COLOR_PIXEL_FORMAT_RGB565,
        .flags.use_dma2d = true,
#endif
        .num_fbs = num_fbs,
        .video_timing = {
            .h_size = panel->h_res,
            .v_size = panel->v_res,
            .hsync_back_porch = mipi->hsync_back_porch,
            .hsync_pulse_width = mipi->hsync_pulse_width,
            .hsync_front_porch = mipi->hsync_front_porch,
            .vsync_back_porch = mipi->vsync_back_porch,
            .vsync_pulse_width = mipi->vsync_pulse_width,
            .vsync_front_porch = mipi->vsync_front_porch,
        },
    };

    static st7701_vendor_config_t vendor;
    vendor.init_cmds = (const st7701_lcd_init_cmd_t *)panel->vendor_init_cmds;
    vendor.init_cmds_size = panel->vendor_init_cmds_count;
    vendor.flags.use_mipi_interface = 1;
    vendor.mipi_config.dsi_bus = g_panellan_lcd_rt.dsi_bus;
    vendor.mipi_config.dpi_config = &dpi_config;

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = mipi->rst,
        .rgb_ele_order = panel->color_order,
        .bits_per_pixel = panel->bits_per_pixel ? panel->bits_per_pixel : 24,
        .vendor_config = &vendor,
    };
    panel_config.flags.reset_active_high = panel->reset_active_high;

    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7701(g_panellan_lcd_rt.io, &panel_config, &g_panellan_lcd_rt.panel),
                        TAG, "st7701");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(g_panellan_lcd_rt.panel), TAG, "reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(g_panellan_lcd_rt.panel), TAG, "init");
    return panellan_lcd_panel_disp_on_and_bl(panel);
#else
    (void)tear;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t panellan_lcd_deinit_mipi_dsi(void) {
    if (g_panellan_lcd_rt.panel) {
        esp_lcd_panel_disp_on_off(g_panellan_lcd_rt.panel, false);
        esp_lcd_panel_del(g_panellan_lcd_rt.panel);
        g_panellan_lcd_rt.panel = NULL;
    }
    if (g_panellan_lcd_rt.io) {
        esp_lcd_panel_io_del(g_panellan_lcd_rt.io);
        g_panellan_lcd_rt.io = NULL;
    }
    if (g_panellan_lcd_rt.dsi_bus) {
        esp_lcd_del_dsi_bus(g_panellan_lcd_rt.dsi_bus);
        g_panellan_lcd_rt.dsi_bus = NULL;
    }
    return ESP_OK;
}

#endif /* MIPI */
