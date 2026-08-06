/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Internal panellan_lcd header (not part of the public API).
 * Applications should include only panellan_lcd.h.
 */

#pragma once

#include "driver/spi_master.h"
#include "esp_lcd_panel_ops.h"
#if CONFIG_PANELLAN_LCD_ENABLE_I80
#include "esp_lcd_io_i80.h"
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_MIPI_DSI
#include "esp_lcd_mipi_dsi.h"
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_RGB
#include "esp_lcd_panel_rgb.h"
#endif

#include "panellan_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LCD runtime state (singleton; owned by panellan_lcd_init / deinit)
 *
 * Bus drivers write panel / io / bus handles on init and clear them on deinit.
 * Fields are Kconfig-gated to match enabled bus capabilities.
 */
typedef struct {
    panellan_lcd_config_t cfg;                  /*!< Copy of board-supplied config */
    esp_lcd_panel_handle_t panel;           /*!< esp_lcd panel handle */
    esp_lcd_panel_io_handle_t io;           /*!< Panel IO (may be NULL for RGB passthrough) */
#if CONFIG_PANELLAN_LCD_ENABLE_I80
    esp_lcd_i80_bus_handle_t i80_bus;       /*!< I80 bus handle */
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_SPI_WITH_PSRAM || CONFIG_PANELLAN_LCD_ENABLE_SPI_WITHOUT_PSRAM || \
    CONFIG_PANELLAN_LCD_ENABLE_QSPI
    bool spi_bus_owned;                     /*!< true: deinit must call spi_bus_free */
    spi_host_device_t spi_host;             /*!< Owned SPI host */
#endif
#if CONFIG_PANELLAN_LCD_ENABLE_MIPI_DSI
    esp_lcd_dsi_bus_handle_t dsi_bus;       /*!< MIPI DSI bus handle */
#endif
    bool active;                            /*!< Successfully inited; not yet deinited */
} panellan_lcd_runtime_t;

/** Global runtime singleton (defined in panellan_lcd.c) */
extern panellan_lcd_runtime_t g_panellan_lcd_rt;

/* ---- Shared helpers (helpers/; reusable by bus inits) ---- */

/** Configure backlight GPIO as output and drive it to the off level */
void panellan_lcd_bl_gpio_init(const panellan_lcd_panel_t *panel);

/**
 * @brief Apply board base orientation + app rotation as panel swap/mirror
 * @param panel Created esp_lcd panel
 * @param pcfg  Board panel config (includes base_swap_xy / base_mirror_*)
 * @param rotation Application rotation
 */
esp_err_t panellan_lcd_apply_orientation(esp_lcd_panel_handle_t panel,
                                     const panellan_lcd_panel_t *pcfg,
                                     panellan_display_rotation_t rotation);

/**
 * @brief Turn panel display output on (DISPON only; no backlight)
 */
esp_err_t panellan_lcd_panel_disp_on(const panellan_lcd_panel_t *pcfg);

/**
 * @brief Enable backlight when turn_on_bl_after_init && bl_gpio valid
 * @note  LVGL path often uses panellan_lcd_set_bl() after adapter is ready instead
 */
esp_err_t panellan_lcd_panel_bl_on(const panellan_lcd_panel_t *pcfg);

/* ---- Bus driver entry points (lcd_drv/; dispatched by panellan_lcd_init) ---- */

#if CONFIG_PANELLAN_LCD_ENABLE_RGB
/** Init RGB parallel bus; tear is tear-avoid mode (for adapter reference) */
esp_err_t panellan_lcd_init_rgb(const panellan_lcd_config_t *cfg,
                            int tear,
                            panellan_display_rotation_t rotation);
/** Free RGB panel / related resources */
esp_err_t panellan_lcd_deinit_rgb(void);
/**
 * @brief Send RGB panel init command table over 9-bit SPI
 * @param cmds  Command array (panellan_lcd_spi9_cmd_t)
 * @param count Command count
 */
esp_err_t panellan_lcd_spi9_run(gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs,
                            spi_host_device_t host,
                            const panellan_lcd_spi9_cmd_t *cmds, size_t count);
#endif

#if CONFIG_PANELLAN_LCD_ENABLE_I80
/** Init Intel 8080 (I80) bus + panel IC */
esp_err_t panellan_lcd_init_i80(const panellan_lcd_config_t *cfg,
                            panellan_display_rotation_t rotation);
/** Free I80 panel / IO / bus and flush-side resources */
esp_err_t panellan_lcd_deinit_i80(void);
/**
 * @brief Free I80 LVGL bounce / semaphore flush resources
 * @note  No-op without LVGL; with LVGL provided by panellan_lcd_i80_flush.c
 */
void panellan_lcd_i80_flush_cleanup(void);
#endif

#if CONFIG_PANELLAN_LCD_ENABLE_SPI_WITH_PSRAM
/** Init SPI bus (draw buffers may live in PSRAM) */
esp_err_t panellan_lcd_init_spi_with_psram(const panellan_lcd_config_t *cfg,
                                       panellan_display_rotation_t rotation);
esp_err_t panellan_lcd_deinit_spi_with_psram(void);
#endif

#if CONFIG_PANELLAN_LCD_ENABLE_SPI_WITHOUT_PSRAM
/** Init SPI bus (no PSRAM / partial-buffer scenarios) */
esp_err_t panellan_lcd_init_spi_without_psram(const panellan_lcd_config_t *cfg,
                                          panellan_display_rotation_t rotation);
esp_err_t panellan_lcd_deinit_spi_without_psram(void);
#endif

#if CONFIG_PANELLAN_LCD_ENABLE_QSPI
/** Init QSPI bus */
esp_err_t panellan_lcd_init_qspi(const panellan_lcd_config_t *cfg,
                             panellan_display_rotation_t rotation);
esp_err_t panellan_lcd_deinit_qspi(void);
#endif

#if CONFIG_PANELLAN_LCD_ENABLE_MIPI_DSI
/** Init MIPI DSI (mostly ESP32-P4); tear is tear-avoid mode */
esp_err_t panellan_lcd_init_mipi_dsi(const panellan_lcd_config_t *cfg,
                                 int tear,
                                 panellan_display_rotation_t rotation);
esp_err_t panellan_lcd_deinit_mipi_dsi(void);
#endif

#ifdef __cplusplus
}
#endif
