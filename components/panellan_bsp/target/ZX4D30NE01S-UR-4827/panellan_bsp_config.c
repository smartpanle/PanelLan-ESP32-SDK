/*
 * SPDX-License-Identifier: Apache-2.0
 * Board L3: ZX4D30NE — RGB passthrough, no touch.
 */

#include "panellan_bsp.h"
#include "panellan_bsp_pin.h"

static const panellan_bsp_config_t s_board_config = {
    .lcd_bus = PANELLAN_LCD_BUS_RGB,
    .lcd_panel_ic = PANELLAN_LCD_PANEL_RGB_ONLY,
    .touch_ic = PANELLAN_TOUCH_IC_NONE,
    .lcd = {
        .bus = PANELLAN_LCD_BUS_RGB,
        .panel = {
            .h_res = HW_LCD_H_RES,
            .v_res = HW_LCD_V_RES,
            .bl_gpio = LCD_BL_PIN,
            .bl_on_level = 1,
            .turn_on_bl_after_init = false,
            .te_gpio = GPIO_NUM_NC,
            .panel_ic = PANELLAN_LCD_PANEL_RGB_ONLY,
            .bits_per_pixel = 16,
        },
        .u.rgb = {
            .vsync = LCD_VSYNC_GPIO,
            .hsync = LCD_HSYNC_GPIO,
            .de = LCD_DE_GPIO,
            .pclk = LCD_PCLK_GPIO,
            .disp = GPIO_NUM_NC,
            .data = {
                LCD_D0_GPIO, LCD_D1_GPIO, LCD_D2_GPIO, LCD_D3_GPIO,
                LCD_D4_GPIO, LCD_D5_GPIO, LCD_D6_GPIO, LCD_D7_GPIO,
                LCD_D8_GPIO, LCD_D9_GPIO, LCD_D10_GPIO, LCD_D11_GPIO,
                LCD_D12_GPIO, LCD_D13_GPIO, LCD_D14_GPIO, LCD_D15_GPIO,
            },
            .pclk_hz = 11 * 1000 * 1000,
            .hsync_pulse_width = 10,
            .hsync_back_porch = 30,
            .hsync_front_porch = 20,
            .vsync_pulse_width = 10,
            .vsync_back_porch = 30,
            .vsync_front_porch = 10,
            .pclk_active_neg = true,
            .clk_src = LCD_CLK_SRC_PLL160M,
            .bounce_mode = PANELLAN_LCD_BOUNCE_AUTO,
            .pulse_vsync_enable = true,
        },
    },
    .touch = PANELLAN_TOUCH_CONFIG_DEFAULT(),
};

const panellan_bsp_config_t *g_panellan_bsp_config = &s_board_config;
