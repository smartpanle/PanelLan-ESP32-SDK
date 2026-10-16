/*
 * SPDX-License-Identifier: Apache-2.0
 * Board L3: ZX4D30CE08S — I80 NV3041 + CST3240.
 */

#include "panellan_bsp.h"
#include "panellan_bsp_pin.h"

static const panellan_lcd_init_cmd_t vendor_specific_init_default[] = {
    {0XFF, (const uint8_t[]){0XA5}, 1, 0},  // Command set selection
    {0XE7, (const uint8_t[]){0X10}, 1, 0},  // TE output enable
    {0X35, (const uint8_t[]){0X01}, 1, 0},  // TE mode
    {0X3A, (const uint8_t[]){0X01}, 1, 0},  // Pixel format: 565 (01=565, 00=666)
    {0X40, (const uint8_t[]){0X01}, 1, 0},  // Panel mode: IPS (01=IPS, 00=TN)
    {0X41, (const uint8_t[]){0X01}, 1, 0},  // Bus width: 8-bit (01=8bit, 03=16bit)
    {0X55, (const uint8_t[]){0X01}, 1, 0},  // CABC enable
    {0X44, (const uint8_t[]){0X15}, 1, 0},  // VBP
    {0X45, (const uint8_t[]){0X15}, 1, 0},  // VFP
    {0X7D, (const uint8_t[]){0X03}, 1, 0},  // Power control
    {0XC1, (const uint8_t[]){0XAB}, 1, 0},  // VCI_VCL
    {0XC2, (const uint8_t[]){0X17}, 1, 0},  // VDV
    {0XC3, (const uint8_t[]){0X10}, 1, 0},  // VCM
    {0XC6, (const uint8_t[]){0X3A}, 1, 0},  // FR setting
    {0XC7, (const uint8_t[]){0X25}, 1, 0},  // Step-up circuit
    {0XC8, (const uint8_t[]){0X11}, 1, 0},  // Stability
    {0X7A, (const uint8_t[]){0X49}, 1, 0},  // Flicker
    {0X6F, (const uint8_t[]){0X2F}, 1, 0},  // User GVDD
    {0X78, (const uint8_t[]){0X4B}, 1, 0},  // DC/DC
    {0X73, (const uint8_t[]){0X08}, 1, 0},  // Panel characteristic
    {0X74, (const uint8_t[]){0X12}, 1, 0},  // Panel characteristic
    {0XC9, (const uint8_t[]){0X00}, 1, 0},  // Gamma adj
    {0X67, (const uint8_t[]){0X11}, 1, 0},  // Gamma adj
    {0X51, (const uint8_t[]){0X20}, 1, 0},  // Gate EQ
    {0X52, (const uint8_t[]){0X7C}, 1, 0},  // Gate EQ
    {0X53, (const uint8_t[]){0X1C}, 1, 0},  // Gate EQ
    {0X54, (const uint8_t[]){0X77}, 1, 0},  // Gate EQ
    {0X46, (const uint8_t[]){0X0A}, 1, 0},  // Source old
    {0X47, (const uint8_t[]){0X2A}, 1, 0},  // Source old
    {0X48, (const uint8_t[]){0X0A}, 1, 0},  // Source old
    {0X49, (const uint8_t[]){0X1A}, 1, 0},  // Source old
    {0X56, (const uint8_t[]){0X43}, 1, 0},  // Source new
    {0X57, (const uint8_t[]){0X42}, 1, 0},  // Source new
    {0X58, (const uint8_t[]){0X3C}, 1, 0},  // Source new
    {0X59, (const uint8_t[]){0X64}, 1, 0},  // Source new
    {0X5A, (const uint8_t[]){0X41}, 1, 0},  // Source new
    {0X5B, (const uint8_t[]){0X3C}, 1, 0},  // Source new
    {0X5C, (const uint8_t[]){0X02}, 1, 0},  // Source new
    {0X5D, (const uint8_t[]){0X3C}, 1, 0},  // Source new
    {0X5E, (const uint8_t[]){0X1F}, 1, 0},  // Source new
    {0X60, (const uint8_t[]){0X80}, 1, 0},  // Source new
    {0X61, (const uint8_t[]){0X3F}, 1, 0},  // Source new
    {0X62, (const uint8_t[]){0X21}, 1, 0},  // Source new
    {0X63, (const uint8_t[]){0X07}, 1, 0},  // Source new
    {0X64, (const uint8_t[]){0XE0}, 1, 0},  // Source new
    {0X65, (const uint8_t[]){0X01}, 1, 0},  // Source new
    {0XCA, (const uint8_t[]){0X20}, 1, 0},  // Positive gamma tuning
    {0XCB, (const uint8_t[]){0X52}, 1, 0},  // Positive gamma tuning
    {0XCC, (const uint8_t[]){0X10}, 1, 0},  // Positive gamma tuning
    {0XCD, (const uint8_t[]){0X42}, 1, 0},  // Positive gamma tuning
    {0XD0, (const uint8_t[]){0X20}, 1, 0},  // Negative gamma tuning
    {0XD1, (const uint8_t[]){0X52}, 1, 0},  // Negative gamma tuning
    {0XD2, (const uint8_t[]){0X10}, 1, 0},  // Negative gamma tuning
    {0XD3, (const uint8_t[]){0X42}, 1, 0},  // Negative gamma tuning
    {0XD4, (const uint8_t[]){0X0A}, 1, 0},  // Gamma tuning
    {0XD5, (const uint8_t[]){0X32}, 1, 0},  // Gamma tuning
    {0XE5, (const uint8_t[]){0X06}, 1, 0},  // Internal timing
    {0XE6, (const uint8_t[]){0X00}, 1, 0},  // Internal timing
    {0X6E, (const uint8_t[]){0X14}, 1, 0},  // Internal timing
    {0X80, (const uint8_t[]){0X04}, 1, 0},  // Gamma P1
    {0XA0, (const uint8_t[]){0X00}, 1, 0},  // Gamma N1
    {0X81, (const uint8_t[]){0X07}, 1, 0},  // Gamma P2
    {0XA1, (const uint8_t[]){0X05}, 1, 0},  // Gamma N2
    {0X82, (const uint8_t[]){0X06}, 1, 0},  // Gamma P3
    {0XA2, (const uint8_t[]){0X04}, 1, 0},  // Gamma N3
    {0X83, (const uint8_t[]){0X39}, 1, 0},  // Gamma P4
    {0XA3, (const uint8_t[]){0X39}, 1, 0},  // Gamma N4
    {0X84, (const uint8_t[]){0X3A}, 1, 0},  // Gamma P5
    {0XA4, (const uint8_t[]){0X3A}, 1, 0},  // Gamma N5
    {0X85, (const uint8_t[]){0X3F}, 1, 0},  // Gamma P6
    {0XA5, (const uint8_t[]){0X3F}, 1, 0},  // Gamma N6
    {0X86, (const uint8_t[]){0X30}, 1, 0},  // Gamma P7
    {0XA6, (const uint8_t[]){0X30}, 1, 0},  // Gamma N7
    {0X87, (const uint8_t[]){0X3A}, 1, 0},  // Gamma P8
    {0XA7, (const uint8_t[]){0X3A}, 1, 0},  // Gamma N8
    {0X88, (const uint8_t[]){0X08}, 1, 0},  // Gamma P9
    {0XA8, (const uint8_t[]){0X08}, 1, 0},  // Gamma N9
    {0X89, (const uint8_t[]){0X0F}, 1, 0},  // Gamma P10
    {0XA9, (const uint8_t[]){0X0F}, 1, 0},  // Gamma N10
    {0X8A, (const uint8_t[]){0X17}, 1, 0},  // Gamma P11
    {0XAA, (const uint8_t[]){0X17}, 1, 0},  // Gamma N11
    {0X8B, (const uint8_t[]){0X10}, 1, 0},  // Gamma P12
    {0XAB, (const uint8_t[]){0X10}, 1, 0},  // Gamma N12
    {0X8C, (const uint8_t[]){0X16}, 1, 0},  // Gamma P13
    {0XAC, (const uint8_t[]){0X16}, 1, 0},  // Gamma N13
    {0X8D, (const uint8_t[]){0X14}, 1, 0},  // Gamma P14
    {0XAD, (const uint8_t[]){0X14}, 1, 0},  // Gamma N14
    {0X8E, (const uint8_t[]){0X11}, 1, 0},  // Gamma P15
    {0XAE, (const uint8_t[]){0X11}, 1, 0},  // Gamma N15
    {0X8F, (const uint8_t[]){0X14}, 1, 0},  // Gamma P16
    {0XAF, (const uint8_t[]){0X14}, 1, 0},  // Gamma N16
    {0X90, (const uint8_t[]){0X06}, 1, 0},  // Gamma P17
    {0XB0, (const uint8_t[]){0X06}, 1, 0},  // Gamma N17
    {0X91, (const uint8_t[]){0X0F}, 1, 0},  // Gamma P18
    {0XB1, (const uint8_t[]){0X0F}, 1, 0},  // Gamma N18
    {0X92, (const uint8_t[]){0X16}, 1, 0},  // Gamma P19
    {0XB2, (const uint8_t[]){0X16}, 1, 0},  // Gamma N19
    {0XFF, (const uint8_t[]){0X00}, 1, 0},  // Exit command set selection
    {0X11, NULL, 0, 120},  // Sleep out (120ms delay)
    {0X29, NULL, 0, 100},  // Display on (100ms delay)
};

static const panellan_bsp_config_t s_board_config = {
    .lcd_bus = PANELLAN_LCD_BUS_I80,
    .lcd_panel_ic = PANELLAN_LCD_PANEL_NV3041,
    .touch_ic = PANELLAN_TOUCH_IC_CST3240,
    .lcd = {
        .bus = PANELLAN_LCD_BUS_I80,
        .panel = {
            .h_res = HW_LCD_H_RES,
            .v_res = HW_LCD_V_RES,
            .bl_gpio = LCD_BL_PIN,
            .bl_on_level = 1,
            .turn_on_bl_after_init = true,//false, /* 背光由应用在 UI 首帧刷新后开启。 */
            .te_gpio = GPIO_NUM_NC,
            .panel_ic = PANELLAN_LCD_PANEL_NV3041,
            .vendor_init_cmds = vendor_specific_init_default,
            .vendor_init_cmds_count =
                sizeof(vendor_specific_init_default) / sizeof(vendor_specific_init_default[0]),
            .color_order = LCD_RGB_ELEMENT_ORDER_RGB,
            .bits_per_pixel = 16,
            .apply_orientation = true,
            .base_swap_xy = false,
            .base_mirror_x = true,
            .base_mirror_y = true,
        },
        .u.i80 = {
            .dc = LCD_RS_PIN,
            .wr = LCD_WR_PIN,
            .cs = LCD_CS_PIN,
            .rst = BOARD_RESET_PIN,
            .data = {
                LCD_D0_PIN, LCD_D1_PIN, LCD_D2_PIN, LCD_D3_PIN,
                LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN,
            },
            .pclk_hz = 16 * 1000 * 1000,
            .bus_width = 8,
            .max_transfer_bytes = HW_LCD_H_RES * HW_LCD_V_RES * sizeof(uint16_t),
            .dma_burst_size = 64,
            .clk_src = LCD_CLK_SRC_PLL160M,
            .manual_reset = true,
            /* Shared BOARD_RESET (GPIO38) with CST3240 (TP_RST=NC). Idle HIGH
             * like tmp hw_init ZX4D30CE; active-high left RST low → I2C NACK. */
            .reset_active_high = false,
            .swap_color_bytes = true,
            .wr_idle_level = 1,
            .trans_queue_depth = 10,
            .call_panel_reset = false,
        },
    },
    .touch = {
        .ic = PANELLAN_TOUCH_IC_CST3240,
        .x_max = HW_LCD_H_RES,
        .y_max = HW_LCD_V_RES,
        .sda = TP_I2C_SDA_PIN,
        .scl = TP_I2C_SCL_PIN,
        .rst = TP_I2C_RST_PIN,
        .intr = TP_I2C_INT_PIN,
        .i2c_port = I2C_NUM_0,
        .i2c_clk_hz = 400000,
        .rotation_type = PANELLAN_TOUCH_ROTATION_STANDARD,
        .int_pullup = false,
    },
};

const panellan_bsp_config_t *g_panellan_bsp_config = &s_board_config;
