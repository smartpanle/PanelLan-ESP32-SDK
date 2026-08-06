/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "panellan_lcd.h"
#include "panellan_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/* From menuconfig → PanelLan config → LCD / Touch / Hardware.
 * Capabilities are derived from enabled options (no separate USE_* hide flags). */
#define HW_LCD_H_RES    CONFIG_PANELLAN_LCD_H_RES
#define HW_LCD_V_RES    CONFIG_PANELLAN_LCD_V_RES

#if CONFIG_PANELLAN_LCD_ENABLE_RGB || CONFIG_PANELLAN_LCD_ENABLE_I80 || \
    CONFIG_PANELLAN_LCD_ENABLE_SPI_WITH_PSRAM || CONFIG_PANELLAN_LCD_ENABLE_SPI_WITHOUT_PSRAM || \
    CONFIG_PANELLAN_LCD_ENABLE_QSPI || CONFIG_PANELLAN_LCD_ENABLE_MIPI_DSI
#define HW_USE_LCD      1
#else
#define HW_USE_LCD      0
#endif

#if CONFIG_PANELLAN_TOUCH_FT5X06 || CONFIG_PANELLAN_TOUCH_GT911 || CONFIG_PANELLAN_TOUCH_CST3240
#define HW_USE_TOUCH    1
#else
#define HW_USE_TOUCH    0
#endif

#ifdef CONFIG_PANELLAN_BOARD_USE_ENCODER
#define HW_USE_ENCODER  1
#else
#define HW_USE_ENCODER  0
#endif

/**
 * L3 board hardware selection + typed configs.
 * lcd.bus / panel_ic / touch.ic are the defaults for this board.
 */
typedef struct {
    panellan_lcd_bus_t lcd_bus;
    panellan_lcd_panel_ic_t lcd_panel_ic;
    panellan_touch_ic_t touch_ic;
    panellan_lcd_config_t lcd;
    panellan_touch_config_t touch;
} panellan_bsp_config_t;

/** Board config pointer; set by board config.c (or panellan_bsp_prepare). */
extern const panellan_bsp_config_t *g_panellan_bsp_config;

/** Optional one-shot HW prep (reset/detect); default weak no-op. */
void panellan_bsp_prepare(void);

esp_err_t hw_lcd_init(esp_lcd_panel_handle_t *panel_handle,
                      esp_lcd_panel_io_handle_t *io_handle,
                      int tear_avoid_mode,
                      panellan_display_rotation_t rotation);
esp_err_t hw_lcd_deinit(void);
int hw_lcd_get_te_gpio(void);

#if HW_USE_TOUCH
#include "esp_lcd_touch.h"

esp_err_t hw_touch_init(esp_lcd_touch_handle_t *touch_handle,
                        panellan_display_rotation_t rotation);

#endif

#if HW_USE_ENCODER
#include "iot_knob.h"
#include "iot_button.h"

const knob_config_t *hw_knob_get_config(void);
button_handle_t hw_knob_get_button(void);

#endif

uint32_t hw_lcd_get_bus_freq_hz(void);
uint8_t hw_lcd_get_bus_data_lines(void);
uint8_t hw_lcd_get_bits_per_pixel(void);

void hw_lcd_set_bl(bool on);

/** Panel DISPON/DISPOFF；建议在首帧 LVGL 刷新后再开，避免白闪 */
esp_err_t hw_lcd_disp_on(bool on);

#if CONFIG_PANELLAN_LCD_ENABLE_I80
void *hw_lcd_i80_alloc_draw_buffer(size_t size, uint32_t caps);
esp_err_t hw_lcd_i80_setup_adapter_flush(void *disp, esp_lcd_panel_io_handle_t io);
esp_err_t hw_lcd_i80_bind_lvgl_draw_buffer(void *disp);
#endif

#ifdef __cplusplus
}
#endif
