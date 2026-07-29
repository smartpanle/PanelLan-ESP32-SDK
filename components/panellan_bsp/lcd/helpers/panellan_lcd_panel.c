/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Panel finish: orientation (swap/mirror/invert) and display-on + backlight.
 */

#include "esp_check.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"

#include "panellan_lcd_priv.h"

static const char *TAG = "panellan_lcd_panel";

/* Compute and apply swap_xy / mirror / invert from board base orientation + LVGL rotation */
esp_err_t panellan_lcd_apply_orientation(esp_lcd_panel_handle_t panel,
                                     const panellan_lcd_panel_t *pcfg,
                                     panellan_display_rotation_t rotation) {
    if (!panel || !pcfg || !pcfg->apply_orientation) {
        return ESP_OK;
    }
    bool swap_xy = false, mirror_x = false, mirror_y = false;
    panellan_lcd_get_orientation_flags(pcfg->base_swap_xy, pcfg->base_mirror_x, pcfg->base_mirror_y,
                                   rotation, &swap_xy, &mirror_x, &mirror_y);
    ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(panel, swap_xy), TAG, "swap_xy");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(panel, mirror_x, mirror_y), TAG, "mirror");
    if (pcfg->invert_color) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel, true), TAG, "invert");
    }
    return ESP_OK;
}

/* Turn on panel display output; enable backlight if configured */
esp_err_t panellan_lcd_panel_disp_on_and_bl(const panellan_lcd_panel_t *pcfg) {
    if (!g_panellan_lcd_rt.panel || !pcfg) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_lcd_panel_disp_on_off(g_panellan_lcd_rt.panel, true);
    if (pcfg->turn_on_bl_after_init && pcfg->bl_gpio != GPIO_NUM_NC) {
        gpio_set_level(pcfg->bl_gpio, pcfg->bl_on_level);
    }
    return ESP_OK;
}
