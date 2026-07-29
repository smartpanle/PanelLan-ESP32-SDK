/*
 * SPDX-License-Identifier: Apache-2.0
 * Board L3: ZX2D10GE01R-V — RGB + SPI9 init, no touch (encoder board).
 */

#include "panellan_bsp.h"
#include "panellan_bsp_pin.h"

static const panellan_lcd_spi9_cmd_t lcd_init_cmds[] = {
    /* ---- Command2 BK0 select ---- */
    {0xFF, (const uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    /* ---- Display line setting (scan line) ---- */
    {0xC0, (const uint8_t[]){0x3B, 0x00}, 2, 0},
    /* ---- Porch control (VBP) ---- */
    {0xC1, (const uint8_t[]){0x0B, 0x02}, 2, 0},
    /* ---- Inversion setting ---- */
    {0xC2, (const uint8_t[]){0x07, 0x02}, 2, 0},
    {0xCC, (const uint8_t[]){0x10}, 1, 0},
    /* ---- RGB format ---- */
    {0xCD, (const uint8_t[]){0x08}, 1, 0},
    /* ---- Positive gamma (IPS) ---- */
    {0xB0, (const uint8_t[]){
         0x00, 0x11, 0x16, 0x0E, 0x11, 0x06, 0x05, 0x09,
         0x08, 0x21, 0x06, 0x13, 0x10, 0x29, 0x31, 0x18
    }, 16, 0},
    /* ---- Negative gamma (IPS) ---- */
    {0xB1, (const uint8_t[]){
         0x00, 0x11, 0x16, 0x0E, 0x11, 0x07, 0x05, 0x09,
         0x09, 0x21, 0x05, 0x13, 0x11, 0x2A, 0x31, 0x18
    }, 16, 0},
    /* ---- Command2 BK1 select ---- */
    {0xFF, (const uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    /* ---- VOP amplitude ---- */
    {0xB0, (const uint8_t[]){0x6D}, 1, 0},
    /* ---- VCOM amplitude ---- */
    {0xB1, (const uint8_t[]){0x37}, 1, 0},
    /* ---- VGH voltage (12V) ---- */
    {0xB2, (const uint8_t[]){0x81}, 1, 0},
    {0xB3, (const uint8_t[]){0x80}, 1, 0},
    /* ---- VGL voltage (-8.3V) ---- */
    {0xB5, (const uint8_t[]){0x43}, 1, 0},
    {0xB7, (const uint8_t[]){0x85}, 1, 0},
    {0xB8, (const uint8_t[]){0x20}, 1, 0},
    {0xC1, (const uint8_t[]){0x78}, 1, 0},
    {0xC2, (const uint8_t[]){0x78}, 1, 0},
    {0xD0, (const uint8_t[]){0x88}, 1, 0},
    /* ---- GIP settings ---- */
    {0xE0, (const uint8_t[]){0x00, 0x00, 0x02}, 3, 0},
    {0xE1, (const uint8_t[]){
         0x03, 0xA0, 0x00, 0x00, 0x04, 0xA0, 0x00, 0x00,
         0x00, 0x20, 0x20
    }, 11, 0},
    {0xE2, (const uint8_t[]){
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00
    }, 13, 0},
    {0xE3, (const uint8_t[]){0x00, 0x00, 0x11, 0x00}, 4, 0},
    {0xE4, (const uint8_t[]){0x22, 0x00}, 2, 0},
    {0xE5, (const uint8_t[]){
         0x05, 0xEC, 0xA0, 0xA0, 0x07, 0xEE, 0xA0, 0xA0,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }, 16, 0},
    {0xE6, (const uint8_t[]){0x00, 0x00, 0x11, 0x00}, 4, 0},
    {0xE7, (const uint8_t[]){0x22, 0x00}, 2, 0},
    {0xE8, (const uint8_t[]){
         0x06, 0xED, 0xA0, 0xA0, 0x08, 0xEF, 0xA0, 0xA0,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }, 16, 0},
    {0xEB, (const uint8_t[]){0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00}, 7, 0},
    {0xED, (const uint8_t[]){
         0xFF, 0xFF, 0xFF, 0xBA, 0x0A, 0xBF, 0x45, 0xFF,
         0xFF, 0x54, 0xFB, 0xA0, 0xAB, 0xFF, 0xFF, 0xFF
    }, 16, 0},
    {0xEF, (const uint8_t[]){0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F}, 6, 0},
    /* ---- Command2 BK3 select ---- */
    {0xFF, (const uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xEF, (const uint8_t[]){0x08}, 1, 0},
    /* ---- Back to user command set (BK0) ---- */
    {0xFF, (const uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},
    /* ---- MADCTL ---- */
    {0x36, (const uint8_t[]){0x00}, 1, 0},
    /* ---- Pixel format: RGB666 (18-bit) ---- */
    {0x3A, (const uint8_t[]){0x66}, 1, 0},
    /* ---- Sleep Out (wake from sleep), 120ms delay ---- */
    {0x11, NULL, 0, 120},
    /* ---- Display ON ---- */
    {0x29, NULL, 0, 0},
};

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
            .disp = LCD_DISP_EN_GPIO,
            .data = {
                LCD_D0_GPIO, LCD_D1_GPIO, LCD_D2_GPIO, LCD_D3_GPIO,
                LCD_D4_GPIO, LCD_D5_GPIO, LCD_D6_GPIO, LCD_D7_GPIO,
                LCD_D8_GPIO, LCD_D9_GPIO, LCD_D10_GPIO, LCD_D11_GPIO,
                LCD_D12_GPIO, LCD_D13_GPIO, LCD_D14_GPIO, LCD_D15_GPIO,
            },
            .pclk_hz = 15 * 1000 * 1000,
            .hsync_pulse_width = 10,
            .hsync_back_porch = 10,
            .hsync_front_porch = 10,
            .vsync_pulse_width = 2,
            .vsync_back_porch = 12,
            .vsync_front_porch = 14,
            .pclk_active_neg = false,
            .clk_src = LCD_CLK_SRC_PLL240M,
            .bounce_mode = PANELLAN_LCD_BOUNCE_AUTO,
            .pulse_vsync_enable = true,
            .use_spi9_init = true,
            .spi9_mosi = LCD_SPI_MOSI,
            .spi9_clk = LCD_SPI_CLK,
            .spi9_cs = LCD_SPI_CS,
            .spi9_host = SPI2_HOST,
            .spi9_cmds = lcd_init_cmds,
            .spi9_cmd_count = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
        },
    },
    .touch = PANELLAN_TOUCH_CONFIG_DEFAULT(),
};

const panellan_bsp_config_t *g_panellan_bsp_config = &s_board_config;
