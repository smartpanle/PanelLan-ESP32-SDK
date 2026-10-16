/*
 * SPDX-License-Identifier: Apache-2.0
 * Board L3: ZX3D50CE02S-USRC — I80 ST7796 (v1/v2 detect) + FT5x06 / GT1151.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"

#include "panellan_bsp.h"
#include "panellan_bsp_pin.h"

/* 板级触摸配置直接跟随通用驱动选择，不依赖 MCU 型号。 */
#if CONFIG_PANELLAN_TOUCH_GT1151
#define ZX3D50_TOUCH_IC PANELLAN_TOUCH_IC_GT1151
#define ZX3D50_TOUCH_INT_PULLUP false
#elif CONFIG_PANELLAN_TOUCH_FT5X06
#define ZX3D50_TOUCH_IC PANELLAN_TOUCH_IC_FT5X06
#define ZX3D50_TOUCH_INT_PULLUP true
#else
#define ZX3D50_TOUCH_IC PANELLAN_TOUCH_IC_NONE
#define ZX3D50_TOUCH_INT_PULLUP false
#endif

static const char *TAG = "board_cfg";

static const panellan_lcd_init_cmd_t s_st7796_init_v1[] = {
    {0x11, (uint8_t[]){0x00}, 0, 120},
    {0x36, (uint8_t[]){0x48}, 1, 0},
    {0x3A, (uint8_t[]){0x55}, 1, 0},
    {0xF0, (uint8_t[]){0xC3}, 1, 0},
    {0xF0, (uint8_t[]){0x96}, 1, 0},
    {0xB4, (uint8_t[]){0x01}, 1, 0},
    {0xB5, (uint8_t[]){0x1E}, 1, 0},
    {0xB7, (uint8_t[]){0xC6}, 1, 0},
    {0xB9, (uint8_t[]){0x02, 0xE0}, 2, 0},
    {0xC0, (uint8_t[]){0x80, 0x16}, 2, 0},
    {0xC1, (uint8_t[]){0x19}, 1, 0},
    {0xC2, (uint8_t[]){0xA7}, 1, 0},
    {0xC5, (uint8_t[]){0x16}, 1, 0},
    {0xE8, (uint8_t[]){0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33}, 8, 0},
    {0xE0, (uint8_t[]){0xF0, 0x07, 0x0D, 0x04, 0x05, 0x14, 0x36, 0x54, 0x4C, 0x38, 0x13, 0x14, 0x2E, 0x34}, 14, 0},
    {0xE1, (uint8_t[]){0xF0, 0x10, 0x14, 0x0E, 0x0C, 0x08, 0x35, 0x44, 0x4C, 0x26, 0x10, 0x12, 0x2C, 0x32}, 14, 0},
    {0xF0, (uint8_t[]){0x3C}, 1, 0},
    {0xF0, (uint8_t[]){0x69}, 1, 120},
    {0x29, (uint8_t[]){0x00}, 0, 0},
    {0x21, (uint8_t[]){0x00}, 0, 0},
    {0x35, (uint8_t[]){0x00}, 0, 0},
};

static const panellan_lcd_init_cmd_t s_st7796_init_v2[] = {
    {0x11, (uint8_t[]){0x00}, 0, 120},
    {0x36, (uint8_t[]){0x48}, 1, 0},
    {0x3A, (uint8_t[]){0x55}, 1, 0},
    {0xF0, (uint8_t[]){0xC3}, 1, 0},
    {0xF0, (uint8_t[]){0x96}, 1, 0},
    {0xB1, (uint8_t[]){0x90, 0x18}, 2, 0},
    {0xB4, (uint8_t[]){0x01}, 1, 0},
    {0xB7, (uint8_t[]){0xC6}, 1, 0},
    {0xB9, (uint8_t[]){0x02, 0xE0}, 2, 0},
    {0xC0, (uint8_t[]){0x00, 0x00}, 2, 0},
    {0xC1, (uint8_t[]){0x16}, 1, 0},
    {0xC6, (uint8_t[]){0x80}, 1, 0},
    {0xC2, (uint8_t[]){0xA7}, 1, 0},
    {0xC5, (uint8_t[]){0x00}, 1, 0},
    {0xE8, (uint8_t[]){0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33}, 8, 0},
    {0xE0, (uint8_t[]){0xF0, 0x09, 0x0F, 0x0D, 0x0D, 0x1C, 0x3D, 0x44, 0x55, 0x39, 0x18, 0x18, 0x36, 0x39}, 14, 0},
    {0xE1, (uint8_t[]){0xF0, 0x09, 0x0F, 0x09, 0x08, 0x01, 0x31, 0x33, 0x46, 0x09, 0x13, 0x13, 0x2A, 0x31}, 14, 0},
    {0xF0, (uint8_t[]){0x3C}, 1, 0},
    {0xF0, (uint8_t[]){0x69}, 1, 120},
    {0x21, (uint8_t[]){0x00}, 0, 0},
    {0x29, (uint8_t[]){0x00}, 0, 0},
};

/* v1 board */
static const panellan_bsp_config_t s_board_config_v1 = {
    .lcd_bus = PANELLAN_LCD_BUS_I80,
    .lcd_panel_ic = PANELLAN_LCD_PANEL_ST7796,
    .touch_ic = ZX3D50_TOUCH_IC,
    .lcd = {
        .bus = PANELLAN_LCD_BUS_I80,
        .panel = {
            .h_res = HW_LCD_H_RES,
            .v_res = HW_LCD_V_RES,
            .bl_gpio = LCD_BL_PIN,
            .bl_on_level = 1,
            .turn_on_bl_after_init = false, /* 背光由应用在 UI 首帧刷新后开启。 */
            .te_gpio = LCD_TE_PIN,
            .panel_ic = PANELLAN_LCD_PANEL_ST7796,
            .vendor_init_cmds = s_st7796_init_v1,
            .vendor_init_cmds_count = sizeof(s_st7796_init_v1) / sizeof(s_st7796_init_v1[0]),
            .color_order = LCD_RGB_ELEMENT_ORDER_BGR,
            .bits_per_pixel = 16,
            .apply_orientation = true,
            .base_swap_xy = false,
            .base_mirror_x = true,
            .base_mirror_y = false,
        },
        .u.i80 = {
            .dc = LCD_RS_PIN,
            .wr = LCD_WR_PIN,
            .cs = LCD_CS_PIN,
            .rst = GPIO_NUM_NC, /* board_reset() via BOARD_RESET_PIN */
            .data = {
                LCD_D0_PIN, LCD_D1_PIN, LCD_D2_PIN, LCD_D3_PIN,
                LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN,
            },
            .pclk_hz = 40 * 1000 * 1000,
            .bus_width = 8,
            .max_transfer_bytes = 18 * 1024 * 2, // 16KB * 2
            .dma_burst_size = 64,
            .clk_src = LCD_CLK_SRC_DEFAULT,
            .manual_reset = false,
            .reset_active_high = false,
            .swap_color_bytes = true,
            .wr_idle_level = 0,
            .trans_queue_depth = 2,
            .call_panel_reset = true,
        },
    },
    .touch = {
        .ic = ZX3D50_TOUCH_IC,
        .x_max = HW_LCD_H_RES,
        .y_max = HW_LCD_V_RES,
        .sda = TP_I2C_SDA_PIN,
        .scl = TP_I2C_SCL_PIN,
        .rst = TP_I2C_RST_PIN,
        .intr = TP_I2C_INT_PIN,
        .i2c_port = I2C_NUM_0,
        .i2c_clk_hz = 400000,
        .rotation_type = PANELLAN_TOUCH_ROTATION_STANDARD,
        .int_pullup = ZX3D50_TOUCH_INT_PULLUP,
    },
};

/* v2 board */
static const panellan_bsp_config_t s_board_config_v2 = {
    .lcd_bus = PANELLAN_LCD_BUS_I80,
    .lcd_panel_ic = PANELLAN_LCD_PANEL_ST7796,
    .touch_ic = ZX3D50_TOUCH_IC,
    .lcd = {
        .bus = PANELLAN_LCD_BUS_I80,
        .panel = {
            .h_res = HW_LCD_H_RES,
            .v_res = HW_LCD_V_RES,
            .bl_gpio = LCD_BL_PIN,
            .bl_on_level = 1,
            .turn_on_bl_after_init = false, /* 背光由应用在 UI 首帧刷新后开启。 */
            .te_gpio = LCD_TE_PIN,
            .panel_ic = PANELLAN_LCD_PANEL_ST7796,
            .vendor_init_cmds = s_st7796_init_v2,
            .vendor_init_cmds_count = sizeof(s_st7796_init_v2) / sizeof(s_st7796_init_v2[0]),
            .color_order = LCD_RGB_ELEMENT_ORDER_BGR,
            .bits_per_pixel = 16,
            .apply_orientation = true,
            .base_swap_xy = false,
            .base_mirror_x = true,
            .base_mirror_y = false,
        },
        .u.i80 = {
            .dc = LCD_RS_PIN,
            .wr = LCD_WR_PIN,
            .cs = LCD_CS_PIN,
            .rst = GPIO_NUM_NC, /* board_reset() via BOARD_RESET_PIN */
            .data = {
                LCD_D0_PIN, LCD_D1_PIN, LCD_D2_PIN, LCD_D3_PIN,
                LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN,
            },
            .pclk_hz = 40 * 1000 * 1000,
            .bus_width = 8,
            .max_transfer_bytes = 18 * 1024 * 2, // 16KB * 2
            .dma_burst_size = 64,
            .clk_src = LCD_CLK_SRC_DEFAULT,
            .manual_reset = false,
            .reset_active_high = false,
            .swap_color_bytes = true,
            .wr_idle_level = 0,
            .trans_queue_depth = 2,
            .call_panel_reset = true,
        },
    },
    .touch = {
        .ic = ZX3D50_TOUCH_IC,
        .x_max = HW_LCD_H_RES,
        .y_max = HW_LCD_V_RES,
        .sda = TP_I2C_SDA_PIN,
        .scl = TP_I2C_SCL_PIN,
        .rst = TP_I2C_RST_PIN,
        .intr = TP_I2C_INT_PIN,
        .i2c_port = I2C_NUM_0,
        .i2c_clk_hz = 400000,
        .rotation_type = PANELLAN_TOUCH_ROTATION_STANDARD,
        .int_pullup = ZX3D50_TOUCH_INT_PULLUP,
    },
};

const panellan_bsp_config_t *g_panellan_bsp_config;

static uint8_t board_detect_screen_ver(void) {
    gpio_config_t cfg = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << LCD_D0_PIN) | (1ULL << LCD_D3_PIN),
    };
    gpio_config(&cfg);
    uint8_t select = (uint8_t)((gpio_get_level(LCD_D0_PIN) << 4) | gpio_get_level(LCD_D3_PIN));
    ESP_LOGI(TAG, "ZX3D50 screen select=0x%02X (%s)", select, select == 0x11 ? "v1" : "v2");
    return select;
}

static void board_reset(void) {
    gpio_config_t rst_cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << BOARD_RESET_PIN,
    };
    ESP_ERROR_CHECK(gpio_config(&rst_cfg));
    gpio_set_level(BOARD_RESET_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level(BOARD_RESET_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
}

void panellan_bsp_prepare(void) {
    static bool ready = false;
    if (ready) {
        return;
    }

    board_reset();
    const uint8_t screen_ver = board_detect_screen_ver();
    g_panellan_bsp_config = (screen_ver == 0x11) ? &s_board_config_v1 : &s_board_config_v2;
    ESP_LOGW(TAG, "panellan_bsp_config: %02x", screen_ver);
    ready = true;
}
