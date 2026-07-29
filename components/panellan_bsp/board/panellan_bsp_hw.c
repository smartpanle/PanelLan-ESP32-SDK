/*
 * SPDX-License-Identifier: Apache-2.0
 * Common L3 hw_* facade over g_panellan_bsp_config + L2 lcd/touch.
 */

#include <assert.h>

#include "panellan_bsp.h"
#include "panellan_lcd.h"
#include "panellan_touch.h"

void __attribute__((weak)) panellan_bsp_prepare(void) {
}

esp_err_t hw_lcd_init(esp_lcd_panel_handle_t *panel_handle,
                      esp_lcd_panel_io_handle_t *io_handle,
                      int tear_avoid_mode,
                      panellan_display_rotation_t rotation) {
    panellan_bsp_prepare();
    assert(g_panellan_bsp_config != NULL);
    return panellan_lcd_init(&g_panellan_bsp_config->lcd, tear_avoid_mode, rotation,
                         panel_handle, io_handle);
}

esp_err_t hw_lcd_deinit(void) {
    return panellan_lcd_deinit();
}

void hw_lcd_set_bl(bool on) {
    panellan_lcd_set_bl(on);
}

int hw_lcd_get_te_gpio(void) {
    return panellan_lcd_get_te_gpio();
}

uint32_t hw_lcd_get_bus_freq_hz(void) {
    return panellan_lcd_get_bus_freq_hz();
}

uint8_t hw_lcd_get_bus_data_lines(void) {
    return panellan_lcd_get_bus_data_lines();
}

uint8_t hw_lcd_get_bits_per_pixel(void) {
    return panellan_lcd_get_bits_per_pixel();
}

#if HW_USE_TOUCH
esp_err_t hw_touch_init(esp_lcd_touch_handle_t *touch_handle,
                        panellan_display_rotation_t rotation) {
    panellan_bsp_prepare();
    assert(g_panellan_bsp_config != NULL);
    return panellan_touch_init(&g_panellan_bsp_config->touch, rotation, touch_handle);
}
#endif

#if CONFIG_PANELLAN_LCD_ENABLE_I80
void *hw_lcd_i80_alloc_draw_buffer(size_t size, uint32_t caps) {
    return panellan_lcd_i80_alloc_draw_buffer(size, caps);
}

esp_err_t hw_lcd_i80_setup_adapter_flush(void *disp, esp_lcd_panel_io_handle_t io) {
    return panellan_lcd_i80_setup_adapter_flush(disp, io);
}

esp_err_t hw_lcd_i80_bind_lvgl_draw_buffer(void *disp) {
    return panellan_lcd_i80_bind_lvgl_draw_buffer(disp);
}
#endif
