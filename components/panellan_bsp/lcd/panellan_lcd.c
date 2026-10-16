/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * panellan_lcd public entry:
 *   - panellan_lcd_init / deinit: dispatch to lcd_drv by cfg->bus
 *   - Backlight, TE, resolution, and bus-parameter queries
 * Bus details: lcd_drv/; orientation helpers: helpers/.
 */

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"

#include "panellan_lcd_priv.h"

static const char *TAG = "panellan_lcd";

panellan_lcd_runtime_t g_panellan_lcd_rt;

/* ---- Backlight ---- */
void panellan_lcd_bl_gpio_init(const panellan_lcd_panel_t *panel) {
    if (!panel || panel->bl_gpio == GPIO_NUM_NC) {
        return;
    }
    gpio_config_t bk = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << panel->bl_gpio,
    };
    ESP_ERROR_CHECK(gpio_config(&bk));
    /* Off first to avoid garbage during init; real on via disp_on_and_bl / set_bl */
    gpio_set_level(panel->bl_gpio, !panel->bl_on_level); /* 兼容高/低电平点亮的背光。 */
}

/* Set backlight */
void panellan_lcd_set_bl(bool on) {
    if (!g_panellan_lcd_rt.active || g_panellan_lcd_rt.cfg.panel.bl_gpio == GPIO_NUM_NC) {
        return;
    }
    gpio_set_level(g_panellan_lcd_rt.cfg.panel.bl_gpio,
                   on ? g_panellan_lcd_rt.cfg.panel.bl_on_level
                      : !g_panellan_lcd_rt.cfg.panel.bl_on_level);
}

esp_err_t panellan_lcd_disp_on(bool on) {
    if (!g_panellan_lcd_rt.active || !g_panellan_lcd_rt.panel) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_lcd_panel_disp_on_off(g_panellan_lcd_rt.panel, on);
}

/* Init LCD */
esp_err_t panellan_lcd_init(const panellan_lcd_config_t *cfg,
                        int tear_avoid_mode,
                        panellan_display_rotation_t rotation,
                        esp_lcd_panel_handle_t *out_panel,
                        esp_lcd_panel_io_handle_t *out_io) {
    ESP_RETURN_ON_FALSE(cfg && out_panel, ESP_ERR_INVALID_ARG, TAG, "null");
    ESP_RETURN_ON_FALSE(!g_panellan_lcd_rt.active, ESP_ERR_INVALID_STATE, TAG, "already init");

    memset(&g_panellan_lcd_rt, 0, sizeof(g_panellan_lcd_rt));
    g_panellan_lcd_rt.cfg = *cfg;
    /* Board may leave TE as 0 when unset; normalize to GPIO_NUM_NC */
    if (g_panellan_lcd_rt.cfg.panel.te_gpio == 0) {
        g_panellan_lcd_rt.cfg.panel.te_gpio = GPIO_NUM_NC;
    }

    esp_err_t ret = ESP_ERR_NOT_SUPPORTED;
    const char *bus_name = "?";

    /* Init LCD for the selected bus type */
    switch (cfg->bus) {
#if CONFIG_PANELLAN_LCD_ENABLE_RGB
    case PANELLAN_LCD_BUS_RGB:
        ret = panellan_lcd_init_rgb(cfg, tear_avoid_mode, rotation);
        bus_name = "RGB";
        break;
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_I80
    case PANELLAN_LCD_BUS_I80:
        ret = panellan_lcd_init_i80(cfg, rotation);
        bus_name = "I80";
        break;
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_SPI_WITH_PSRAM
    case PANELLAN_LCD_BUS_SPI_WITH_PSRAM:
        ret = panellan_lcd_init_spi_with_psram(cfg, rotation);
        bus_name = "SPI+PSRAM";
        break;
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_SPI_WITHOUT_PSRAM
    case PANELLAN_LCD_BUS_SPI_WITHOUT_PSRAM:
        ret = panellan_lcd_init_spi_without_psram(cfg, rotation);
        bus_name = "SPI-noPSRAM";
        break;
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_QSPI
    case PANELLAN_LCD_BUS_QSPI:
        ret = panellan_lcd_init_qspi(cfg, rotation);
        bus_name = "QSPI";
        break;
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_MIPI_DSI
    case PANELLAN_LCD_BUS_MIPI_DSI:
        ret = panellan_lcd_init_mipi_dsi(cfg, tear_avoid_mode, rotation);
        bus_name = "MIPI-DSI";
        break;
#endif
    default:
        /* Bus not enabled in Kconfig, or invalid bus enum */
        ret = ESP_ERR_NOT_SUPPORTED;
        break;
    }

    if (ret != ESP_OK) {
        /* Roll back partial init (bus deinit tolerates incomplete handles) */
        panellan_lcd_deinit();
        return ret;
    }

    g_panellan_lcd_rt.active = true;
    *out_panel = g_panellan_lcd_rt.panel;
    if (out_io) {
        *out_io = g_panellan_lcd_rt.io;
    }
    ESP_LOGI(TAG, "LCD ready %dx%d bus=%s", cfg->panel.h_res, cfg->panel.v_res, bus_name);
    return ESP_OK;
}

/* Release LCD */
esp_err_t panellan_lcd_deinit(void) {
    esp_err_t ret = ESP_OK;
    const panellan_lcd_bus_t bus = g_panellan_lcd_rt.cfg.bus;

    switch (bus) {
#if CONFIG_PANELLAN_LCD_ENABLE_RGB
    case PANELLAN_LCD_BUS_RGB:
        ret = panellan_lcd_deinit_rgb();
        break;
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_I80
    case PANELLAN_LCD_BUS_I80:
        ret = panellan_lcd_deinit_i80();
        break;
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_SPI_WITH_PSRAM
    case PANELLAN_LCD_BUS_SPI_WITH_PSRAM:
        ret = panellan_lcd_deinit_spi_with_psram();
        break;
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_SPI_WITHOUT_PSRAM
    case PANELLAN_LCD_BUS_SPI_WITHOUT_PSRAM:
        ret = panellan_lcd_deinit_spi_without_psram();
        break;
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_QSPI
    case PANELLAN_LCD_BUS_QSPI:
        ret = panellan_lcd_deinit_qspi();
        break;
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_MIPI_DSI
    case PANELLAN_LCD_BUS_MIPI_DSI:
        ret = panellan_lcd_deinit_mipi_dsi();
        break;
#endif

    default:
        /* Fallback free: init rollback or bus not compiled into firmware */
        if (g_panellan_lcd_rt.panel) {
            esp_lcd_panel_disp_on_off(g_panellan_lcd_rt.panel, false);
            esp_lcd_panel_del(g_panellan_lcd_rt.panel);
            g_panellan_lcd_rt.panel = NULL;
        }
        /* Free LCD IO */
        if (g_panellan_lcd_rt.io) {
            esp_lcd_panel_io_del(g_panellan_lcd_rt.io);
            g_panellan_lcd_rt.io = NULL;
        }
        break;
    }

    if (g_panellan_lcd_rt.cfg.panel.bl_gpio != GPIO_NUM_NC && g_panellan_lcd_rt.active) {
        gpio_set_level(g_panellan_lcd_rt.cfg.panel.bl_gpio, !g_panellan_lcd_rt.cfg.panel.bl_on_level);
    }
    memset(&g_panellan_lcd_rt, 0, sizeof(g_panellan_lcd_rt));
    return ret;
}

/* ---- Runtime queries (for board / LVGL adapter setup) ---- */

int panellan_lcd_get_te_gpio(void) {
    if (!g_panellan_lcd_rt.active) {
        return GPIO_NUM_NC;
    }
    return g_panellan_lcd_rt.cfg.panel.te_gpio;
}

esp_lcd_panel_handle_t panellan_lcd_get_panel(void) {
    return g_panellan_lcd_rt.panel;
}

esp_lcd_panel_io_handle_t panellan_lcd_get_io(void) {
    return g_panellan_lcd_rt.io;
}

uint16_t panellan_lcd_get_h_res(void) {
    return (uint16_t)g_panellan_lcd_rt.cfg.panel.h_res;
}

uint16_t panellan_lcd_get_v_res(void) {
    return (uint16_t)g_panellan_lcd_rt.cfg.panel.v_res;
}

uint32_t panellan_lcd_get_bus_freq_hz(void) {
    if (!g_panellan_lcd_rt.active) {
        return 0;
    }
    switch (g_panellan_lcd_rt.cfg.bus) {
#if CONFIG_PANELLAN_LCD_ENABLE_I80
    case PANELLAN_LCD_BUS_I80:
        return g_panellan_lcd_rt.cfg.u.i80.pclk_hz;
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_RGB
    case PANELLAN_LCD_BUS_RGB:
        return g_panellan_lcd_rt.cfg.u.rgb.pclk_hz;
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_SPI_WITH_PSRAM || CONFIG_PANELLAN_LCD_ENABLE_SPI_WITHOUT_PSRAM
    case PANELLAN_LCD_BUS_SPI_WITH_PSRAM:
    case PANELLAN_LCD_BUS_SPI_WITHOUT_PSRAM:
        return g_panellan_lcd_rt.cfg.u.spi.pclk_hz;
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_QSPI
    case PANELLAN_LCD_BUS_QSPI:
        return g_panellan_lcd_rt.cfg.u.qspi.pclk_hz;
#endif
    default:
        return 0;
    }
}

uint8_t panellan_lcd_get_bus_data_lines(void) {
    if (!g_panellan_lcd_rt.active) {
        return 0;
    }
    switch (g_panellan_lcd_rt.cfg.bus) {
#if CONFIG_PANELLAN_LCD_ENABLE_I80
    case PANELLAN_LCD_BUS_I80:
        return g_panellan_lcd_rt.cfg.u.i80.bus_width ? g_panellan_lcd_rt.cfg.u.i80.bus_width : 8;
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_QSPI
    case PANELLAN_LCD_BUS_QSPI:
        return 4;
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_SPI_WITH_PSRAM || CONFIG_PANELLAN_LCD_ENABLE_SPI_WITHOUT_PSRAM
    case PANELLAN_LCD_BUS_SPI_WITH_PSRAM:
    case PANELLAN_LCD_BUS_SPI_WITHOUT_PSRAM:
        return 1;
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_RGB
    case PANELLAN_LCD_BUS_RGB:
        return 16; /* Common RGB565 parallel width; follow board wiring */
#endif
    default:
        return 0;
    }
}

uint8_t panellan_lcd_get_bits_per_pixel(void) {
    if (!g_panellan_lcd_rt.active) {
        return 16;
    }
    return g_panellan_lcd_rt.cfg.panel.bits_per_pixel ? g_panellan_lcd_rt.cfg.panel.bits_per_pixel : 16;
}
