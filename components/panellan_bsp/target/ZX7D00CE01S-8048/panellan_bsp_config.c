/*
 * SPDX-License-Identifier: Apache-2.0
 * Board L3: ZX7D00 — RGB passthrough + GT911.
 */

#include "panellan_bsp.h"
#include "panellan_bsp_pin.h"

static const panellan_bsp_config_t s_board_config = {
    .lcd_bus = PANELLAN_LCD_BUS_RGB,
    .lcd_panel_ic = PANELLAN_LCD_PANEL_RGB_ONLY,
    .touch_ic = PANELLAN_TOUCH_IC_GT911,
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
            .disp = LCD_DISP_EN_GPIO,
            .data = {
                LCD_D0_GPIO, LCD_D1_GPIO, LCD_D2_GPIO, LCD_D3_GPIO,
                LCD_D4_GPIO, LCD_D5_GPIO, LCD_D6_GPIO, LCD_D7_GPIO,
                LCD_D8_GPIO, LCD_D9_GPIO, LCD_D10_GPIO, LCD_D11_GPIO,
                LCD_D12_GPIO, LCD_D13_GPIO, LCD_D14_GPIO, LCD_D15_GPIO,
            },
            .pclk_hz = 22 * 1000 * 1000,
            .hsync_pulse_width = 1,
            .hsync_back_porch = 87,
            .hsync_front_porch = 20,
            .vsync_pulse_width = 1,
            .vsync_back_porch = 31,
            .vsync_front_porch = 5,
            .pclk_active_neg = false,
            .clk_src = LCD_CLK_SRC_PLL160M,
            .bounce_mode = PANELLAN_LCD_BOUNCE_AUTO,
            .pulse_vsync_enable = true,
        },
    },
    .touch = {
        .ic = PANELLAN_TOUCH_IC_GT911,
        .x_max = HW_LCD_H_RES,
        .y_max = HW_LCD_V_RES,
        .sda = TP_I2C_SDA_PIN,
        .scl = TP_I2C_SCL_PIN,
        .rst = TP_I2C_RST_PIN,
        .intr = TP_I2C_INT_PIN,
        .i2c_port = I2C_NUM_0,
        .i2c_clk_hz = 400000,
        .rotation_type = PANELLAN_TOUCH_ROTATION_STANDARD,
    },
};

const panellan_bsp_config_t *g_panellan_bsp_config = &s_board_config;
