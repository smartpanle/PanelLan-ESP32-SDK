/*
 * SPDX-License-Identifier: Apache-2.0
 * Board L3: ZX2D80 — SPI ST7789 + FT5x06.
 */

#include "panellan_bsp.h"
#include "panellan_bsp_pin.h"
#include "st7789_vendor.h"

static const st7789_lcd_init_cmd_t vendor_specific_init_default[] = {
    {0x11, 0, 0, 120},
    {0x36, (uint8_t[]){0x00}, 1, 0},
    {0x3A, (uint8_t[]){0x05}, 1, 0},
    {0xB2, (uint8_t[]){0x0C, 0x0C, 0x00, 0x33, 0x33}, 5, 0},
    {0xB7, (uint8_t[]){0x46}, 1, 0},
    {0xBB, (uint8_t[]){0x1B}, 1, 0},
    {0xC0, (uint8_t[]){0x2C}, 1, 0},
    {0xC2, (uint8_t[]){0x01}, 1, 0},
    {0xC3, (uint8_t[]){0x0F}, 1, 0},
    {0xC4, (uint8_t[]){0x20}, 1, 0},
    {0xC6, (uint8_t[]){0x0F}, 1, 0},
    {0xD0, (uint8_t[]){0xA7, 0xA1}, 2, 0},
    {0xD0, (uint8_t[]){0xA4, 0xA1}, 2, 0},
    {0xD6, (uint8_t[]){0xA1}, 1, 0},
    {0xE0, (uint8_t[]){
        0xF0, 0x00, 0x06, 0x04, 0x05, 0x05,
        0x31, 0x44, 0x48, 0x36, 0x12, 0x12,
        0x2B, 0x34
    }, 14, 0},
    {0xE1, (uint8_t[]){
        0xF0, 0x0B, 0x0F, 0x0F, 0x0D, 0x26,
        0x31, 0x43, 0x47, 0x38, 0x14, 0x14,
        0x2C, 0x32
    }, 14, 0},
    {0x21, 0, 0, 0},
    {0x29, 0, 0, 0},
    {0x2C, 0, 0, 0},
};

static const panellan_bsp_config_t s_board_config = {
    .lcd_bus = PANELLAN_LCD_BUS_SPI_WITH_PSRAM,
    .lcd_panel_ic = PANELLAN_LCD_PANEL_ST7789,
    .touch_ic = PANELLAN_TOUCH_IC_FT5X06,
    .lcd = {
        .bus = PANELLAN_LCD_BUS_SPI_WITH_PSRAM,
        .panel = {
            .h_res = HW_LCD_H_RES,
            .v_res = HW_LCD_V_RES,
            .bl_gpio = LCD_BL_PIN,
            .bl_on_level = 1,
            .turn_on_bl_after_init = true,
            .te_gpio = GPIO_NUM_NC,
            .panel_ic = PANELLAN_LCD_PANEL_ST7789,
            .vendor_init_cmds = vendor_specific_init_default,
            .vendor_init_cmds_count =
                sizeof(vendor_specific_init_default) / sizeof(vendor_specific_init_default[0]),
            .color_order = LCD_RGB_ELEMENT_ORDER_RGB,
            .bits_per_pixel = 16,
            .apply_orientation = false,
            .base_swap_xy = false,
            .base_mirror_x = false,
            .base_mirror_y = false,
        },
        .u.spi = {
            .host = SPI2_HOST,
            .sclk = LCD_SCL_PIN,
            .mosi = LCD_SDA_PIN,
            .miso = -1,
            .cs   = LCD_CS_PIN,
            .dc   = LCD_RS_PIN,
            .rst  = LCD_RST_PIN,
            .pclk_hz = 40 * 1000 * 1000,
            .max_transfer_bytes = HW_LCD_H_RES * 80 * sizeof(uint16_t),
            .trans_queue_depth = 3,
            .spi_mode = 0,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8
        },
    },
    .touch = {
        .ic = PANELLAN_TOUCH_IC_FT5X06,
        .x_max = HW_LCD_H_RES,
        .y_max = HW_LCD_V_RES,
        .sda = TP_I2C_SDA_PIN,
        .scl = TP_I2C_SCL_PIN,
        .rst = TP_I2C_RST_PIN,
        .intr = TP_I2C_INT_PIN,
        .i2c_port = I2C_NUM_0,
        .i2c_clk_hz = 400000,
        .rotation_type = PANELLAN_TOUCH_ROTATION_STANDARD,
        .int_pullup = true,
    },
};

const panellan_bsp_config_t *g_panellan_bsp_config = &s_board_config;
