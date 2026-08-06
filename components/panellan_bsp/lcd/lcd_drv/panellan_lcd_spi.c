/*
 * SPDX-License-Identifier: Apache-2.0
 * SPI LCD path (with / without PSRAM) — mirrors tmp_sdk lcd_init_spi_*.c
 */

#include "sdkconfig.h"

#if CONFIG_PANELLAN_LCD_ENABLE_SPI_WITH_PSRAM || CONFIG_PANELLAN_LCD_ENABLE_SPI_WITHOUT_PSRAM

#include "esp_check.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "driver/spi_master.h"
#if CONFIG_PANELLAN_LCD_PANEL_ILI9341
#include "esp_lcd_ili9341.h"
#endif
#if CONFIG_PANELLAN_LCD_PANEL_GC9A01
#include "esp_lcd_gc9a01.h"
#endif
#if CONFIG_PANELLAN_LCD_PANEL_ST7789
#include "esp_lcd_panel_st7789.h"
#include "st7789_vendor.h"
#endif

#include "panellan_lcd_priv.h"

static const char *TAG = "panellan_lcd_spi";

static esp_err_t panellan_lcd_init_spi_common(const panellan_lcd_config_t *cfg,
                                          panellan_display_rotation_t rotation) {
    const panellan_lcd_spi_cfg_t *spi = &cfg->u.spi;
    const panellan_lcd_panel_t *panel = &cfg->panel;
    const spi_host_device_t host = spi->host ? spi->host : SPI2_HOST;
    bool run_post_init = false;
#if CONFIG_PANELLAN_LCD_PANEL_ST7789
    st7789_vendor_config_t st7789_vendor = {0};
#endif

    panellan_lcd_bl_gpio_init(panel);

    const spi_bus_config_t buscfg = {
        .sclk_io_num = spi->sclk,
        .mosi_io_num = spi->mosi,
        .miso_io_num = spi->miso,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = (int)spi->max_transfer_bytes,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO), TAG, "spi bus");
    g_panellan_lcd_rt.spi_bus_owned = true;
    g_panellan_lcd_rt.spi_host = host;

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = spi->dc,
        .cs_gpio_num = spi->cs,
        .pclk_hz = spi->pclk_hz,
        .lcd_cmd_bits = spi->lcd_cmd_bits ? spi->lcd_cmd_bits : 8,
        .lcd_param_bits = spi->lcd_param_bits ? spi->lcd_param_bits : 8,
        .spi_mode = spi->spi_mode,
        .trans_queue_depth = spi->trans_queue_depth ? spi->trans_queue_depth : 10,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)host, &io_config, &g_panellan_lcd_rt.io),
                        TAG, "spi io");

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = spi->rst,
        .rgb_ele_order = panel->color_order,
        .bits_per_pixel = panel->bits_per_pixel ? panel->bits_per_pixel : 16,
    };
    panel_config.flags.reset_active_high = panel->reset_active_high;

    switch (panel->panel_ic) {
#if CONFIG_PANELLAN_LCD_PANEL_ILI9341
    case PANELLAN_LCD_PANEL_ILI9341: {
        static ili9341_vendor_config_t vendor;
        vendor.init_cmds = (const ili9341_lcd_init_cmd_t *)panel->vendor_init_cmds;
        vendor.init_cmds_size = panel->vendor_init_cmds_count;
        panel_config.vendor_config = &vendor;
        ESP_RETURN_ON_ERROR(esp_lcd_new_panel_ili9341(g_panellan_lcd_rt.io, &panel_config, &g_panellan_lcd_rt.panel),
                            TAG, "ili9341");
        break;
    }
#endif
#if CONFIG_PANELLAN_LCD_PANEL_GC9A01
    case PANELLAN_LCD_PANEL_GC9A01:
        ESP_RETURN_ON_ERROR(esp_lcd_new_panel_gc9a01(g_panellan_lcd_rt.io, &panel_config, &g_panellan_lcd_rt.panel),
                            TAG, "gc9a01");
        break;
#endif
#if CONFIG_PANELLAN_LCD_PANEL_ST7789
    case PANELLAN_LCD_PANEL_ST7789: {
        st7789_vendor.init_cmds = (const st7789_lcd_init_cmd_t *)panel->vendor_init_cmds;
        st7789_vendor.init_cmds_size = (uint16_t)panel->vendor_init_cmds_count;
        run_post_init = st7789_vendor.init_cmds && st7789_vendor.init_cmds_size;
        ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(g_panellan_lcd_rt.io, &panel_config, &g_panellan_lcd_rt.panel),
                            TAG, "st7789");
        break;
    }
#endif
    default:
        return ESP_ERR_NOT_SUPPORTED;
    }

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(g_panellan_lcd_rt.panel), TAG, "reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(g_panellan_lcd_rt.panel), TAG, "init");
#if CONFIG_PANELLAN_LCD_PANEL_ST7789
    if (run_post_init) {
        ESP_RETURN_ON_ERROR(panel_st7789_init(g_panellan_lcd_rt.io, &st7789_vendor), TAG, "st7789 vendor");
    }
#endif
    ESP_RETURN_ON_ERROR(panellan_lcd_apply_orientation(g_panellan_lcd_rt.panel, panel, rotation), TAG, "orient");
    ESP_RETURN_ON_ERROR(panellan_lcd_panel_disp_on(panel), TAG, "disp_on");
    ESP_RETURN_ON_ERROR(panellan_lcd_panel_bl_on(panel), TAG, "bl_on");
    return ESP_OK;
}

static esp_err_t panellan_lcd_deinit_spi_common(void) {
    if (g_panellan_lcd_rt.panel) {
        esp_lcd_panel_disp_on_off(g_panellan_lcd_rt.panel, false);
        esp_lcd_panel_del(g_panellan_lcd_rt.panel);
        g_panellan_lcd_rt.panel = NULL;
    }
    if (g_panellan_lcd_rt.io) {
        esp_lcd_panel_io_del(g_panellan_lcd_rt.io);
        g_panellan_lcd_rt.io = NULL;
    }
    if (g_panellan_lcd_rt.spi_bus_owned) {
        spi_bus_free(g_panellan_lcd_rt.spi_host);
        g_panellan_lcd_rt.spi_bus_owned = false;
    }
    return ESP_OK;
}

#if CONFIG_PANELLAN_LCD_ENABLE_SPI_WITH_PSRAM
esp_err_t panellan_lcd_init_spi_with_psram(const panellan_lcd_config_t *cfg,
                                       panellan_display_rotation_t rotation) {
    return panellan_lcd_init_spi_common(cfg, rotation);
}

esp_err_t panellan_lcd_deinit_spi_with_psram(void) {
    return panellan_lcd_deinit_spi_common();
}
#elif CONFIG_PANELLAN_LCD_ENABLE_SPI_WITHOUT_PSRAM
esp_err_t panellan_lcd_init_spi_without_psram(const panellan_lcd_config_t *cfg,
                                          panellan_display_rotation_t rotation) {
    return panellan_lcd_init_spi_common(cfg, rotation);
}

esp_err_t panellan_lcd_deinit_spi_without_psram(void) {
    return panellan_lcd_deinit_spi_common();
}
#endif

#endif /* SPI */
