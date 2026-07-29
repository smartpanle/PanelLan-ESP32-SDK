/*
 * SPDX-License-Identifier: Apache-2.0
 * QSPI LCD — mirrors tmp_sdk lcd_init_qspi.c
 */

#include "sdkconfig.h"

#if CONFIG_PANELLAN_LCD_ENABLE_QSPI

#include "esp_check.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#if CONFIG_PANELLAN_LCD_PANEL_ST77916
#include "esp_lcd_st77916.h"
#endif

#include "panellan_lcd_priv.h"

static const char *TAG = "panellan_lcd_qspi";

esp_err_t panellan_lcd_init_qspi(const panellan_lcd_config_t *cfg,
                             panellan_display_rotation_t rotation) {
    const panellan_lcd_qspi_cfg_t *qspi = &cfg->u.qspi;
    const panellan_lcd_panel_t *panel = &cfg->panel;
    const spi_host_device_t host = qspi->host ? qspi->host : SPI2_HOST;

    panellan_lcd_bl_gpio_init(panel);

    const spi_bus_config_t buscfg = {
        .sclk_io_num = qspi->sclk,
        .data0_io_num = qspi->data0,
        .data1_io_num = qspi->data1,
        .data2_io_num = qspi->data2,
        .data3_io_num = qspi->data3,
        .max_transfer_sz = (int)qspi->max_transfer_bytes,
        .flags = SPICOMMON_BUSFLAG_QUAD,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO), TAG, "qspi bus");
    g_panellan_lcd_rt.spi_bus_owned = true;
    g_panellan_lcd_rt.spi_host = host;

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = qspi->dc,
        .cs_gpio_num = qspi->cs,
        .pclk_hz = qspi->pclk_hz,
        .lcd_cmd_bits = qspi->lcd_cmd_bits ? qspi->lcd_cmd_bits : 32,
        .lcd_param_bits = qspi->lcd_param_bits ? qspi->lcd_param_bits : 8,
        .spi_mode = 0,
        .trans_queue_depth = qspi->trans_queue_depth ? qspi->trans_queue_depth : 1,
        .flags.quad_mode = 1,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)host, &io_config, &g_panellan_lcd_rt.io),
                        TAG, "qspi io");

    if (qspi->power_gpio != GPIO_NUM_NC) {
        gpio_config_t pwr = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << qspi->power_gpio,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&pwr), TAG, "power gpio");
        gpio_set_level(qspi->power_gpio, qspi->power_on_level);
    }

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = qspi->rst,
        .rgb_ele_order = panel->color_order,
        .bits_per_pixel = panel->bits_per_pixel ? panel->bits_per_pixel : 16,
    };
    panel_config.flags.reset_active_high = panel->reset_active_high;

    switch (panel->panel_ic) {
#if CONFIG_PANELLAN_LCD_PANEL_ST77916
    case PANELLAN_LCD_PANEL_ST77916: {
        static st77916_vendor_config_t vendor;
        vendor.init_cmds = (const st77916_lcd_init_cmd_t *)panel->vendor_init_cmds;
        vendor.init_cmds_size = panel->vendor_init_cmds_count;
        vendor.flags.use_qspi_interface = 1;
        panel_config.vendor_config = &vendor;
        ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st77916(g_panellan_lcd_rt.io, &panel_config, &g_panellan_lcd_rt.panel),
                            TAG, "st77916");
        break;
    }
#endif
    default:
        return ESP_ERR_NOT_SUPPORTED;
    }

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(g_panellan_lcd_rt.panel), TAG, "reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(g_panellan_lcd_rt.panel), TAG, "init");
    ESP_RETURN_ON_ERROR(panellan_lcd_apply_orientation(g_panellan_lcd_rt.panel, panel, rotation), TAG, "orient");
    return panellan_lcd_panel_disp_on_and_bl(panel);
}

esp_err_t panellan_lcd_deinit_qspi(void) {
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
    if (g_panellan_lcd_rt.cfg.u.qspi.power_gpio != GPIO_NUM_NC) {
        gpio_set_level(g_panellan_lcd_rt.cfg.u.qspi.power_gpio,
                       !g_panellan_lcd_rt.cfg.u.qspi.power_on_level);
    }
    return ESP_OK;
}

#endif /* QSPI */
