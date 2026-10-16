/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"

#if CONFIG_PANELLAN_LCD_ENABLE_I80

#include <stdlib.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_lcd_io_i80.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#if CONFIG_PANELLAN_LCD_PANEL_ST7789
#include "esp_lcd_panel_st7789.h"
#include "st7789_vendor.h"
#endif
#if CONFIG_PANELLAN_LCD_PANEL_ST7796
#include "esp_lcd_st7796.h"
#endif
#if CONFIG_PANELLAN_LCD_PANEL_NV3041
#include "esp_lcd_nv3041.h"
#endif

#include "panellan_lcd_priv.h"

static const char *TAG = "panellan_lcd_i80";

/**
 * @brief 在背光开启前将 I80 屏幕的 RGB565 显存清为黑色（0x000000）。
 *
 * @param cfg 屏幕配置，使用未旋转的物理分辨率。
 * @return ESP_OK 表示清屏完成，否则返回分配或传输错误。
 */
static esp_err_t panellan_lcd_i80_clear_black(const panellan_lcd_config_t *cfg)
{
    const int strip_rows = 16; /* 分块传输，避免分配整帧缓冲区。 */
    const size_t strip_bytes = (size_t)cfg->panel.h_res * strip_rows * sizeof(uint16_t);
    void *black_buffer = esp_lcd_i80_alloc_draw_buffer(g_panellan_lcd_rt.io, strip_bytes,
                                                     MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    ESP_RETURN_ON_FALSE(black_buffer, ESP_ERR_NO_MEM, TAG, "black buffer");
    memset(black_buffer, 0, strip_bytes); /* RGB565 的 0x0000 对应 RGB888 的 0x000000。 */

    esp_err_t ret = esp_lcd_panel_swap_xy(g_panellan_lcd_rt.panel, false);
    for (int y = 0; ret == ESP_OK && y < cfg->panel.v_res; y += strip_rows) {
        const int end_y = (y + strip_rows < cfg->panel.v_res) ? y + strip_rows : cfg->panel.v_res;
        ret = esp_lcd_panel_draw_bitmap(g_panellan_lcd_rt.panel, 0, y, cfg->panel.h_res, end_y, black_buffer);
    }

    /* I80 参数发送会等待所有颜色 DMA 完成；发送 NOP 后才能释放缓冲区和亮背光。 */
    esp_err_t wait_ret = esp_lcd_panel_io_tx_param(g_panellan_lcd_rt.io, 0x00, NULL, 0);
    free(black_buffer);
    ESP_RETURN_ON_ERROR(ret, TAG, "clear black");
    ESP_RETURN_ON_ERROR(wait_ret, TAG, "wait black frame");
    ESP_LOGI(TAG, "Black frame ready before backlight on");
    return ESP_OK;
}

esp_err_t panellan_lcd_init_i80(const panellan_lcd_config_t *cfg,
                            panellan_display_rotation_t rotation) {
    const panellan_lcd_i80_cfg_t *i80 = &cfg->u.i80;
    const panellan_lcd_panel_t *panel = &cfg->panel;

    if (i80->manual_reset && i80->rst >= 0) {
        gpio_config_t rst_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << i80->rst,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&rst_conf), TAG, "rst gpio");
        const int active = i80->reset_active_high ? 1 : 0;
        const int idle = !active;
        /* Align with tmp ZX2D80: assert → settle → release (shared RST w/ touch) */
        gpio_set_level(i80->rst, idle);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(i80->rst, active);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(i80->rst, idle);
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    panellan_lcd_bl_gpio_init(panel);

    gpio_config_t wr_idle = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << i80->wr,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&wr_idle), TAG, "wr gpio");
    gpio_set_level(i80->wr, i80->wr_idle_level ? 1 : 0);

    esp_lcd_i80_bus_config_t bus_config = {
        .dc_gpio_num = i80->dc,
        .wr_gpio_num = i80->wr,
        .clk_src = i80->clk_src ? i80->clk_src : LCD_CLK_SRC_DEFAULT,
        .data_gpio_nums = {
            i80->data[0], i80->data[1], i80->data[2], i80->data[3],
            i80->data[4], i80->data[5], i80->data[6], i80->data[7],
        },
        .bus_width = i80->bus_width ? i80->bus_width : 8,
        .max_transfer_bytes = i80->max_transfer_bytes,
        .dma_burst_size = i80->dma_burst_size ? i80->dma_burst_size : 64,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_i80_bus(&bus_config, &g_panellan_lcd_rt.i80_bus), TAG, "i80 bus");

    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = i80->cs,
        .pclk_hz = i80->pclk_hz,
        .trans_queue_depth = i80->trans_queue_depth ? i80->trans_queue_depth : 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .flags.swap_color_bytes = i80->swap_color_bytes ? 1 : 0,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i80(g_panellan_lcd_rt.i80_bus, &io_config, &g_panellan_lcd_rt.io),
                        TAG, "i80 io");

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = i80->rst,
        .rgb_ele_order = panel->color_order,
        .bits_per_pixel = panel->bits_per_pixel ? panel->bits_per_pixel : 16,
    };
    panel_config.flags.reset_active_high = i80->reset_active_high;

    switch (panel->panel_ic) {
#if CONFIG_PANELLAN_LCD_PANEL_ST7796
    case PANELLAN_LCD_PANEL_ST7796: {
        static st7796_vendor_config_t vendor;
        vendor.init_cmds = (const st7796_lcd_init_cmd_t *)panel->vendor_init_cmds;
        vendor.init_cmds_size = panel->vendor_init_cmds_count;
        panel_config.vendor_config = &vendor;
        ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7796(g_panellan_lcd_rt.io, &panel_config, &g_panellan_lcd_rt.panel),
                            TAG, "st7796");
        break;
    }
#endif
#if CONFIG_PANELLAN_LCD_PANEL_NV3041
    case PANELLAN_LCD_PANEL_NV3041: {
        static nv3041_vendor_config_t vendor;
        vendor.init_cmds = (const nv3041_lcd_init_cmd_t *)panel->vendor_init_cmds;
        vendor.init_cmds_size = panel->vendor_init_cmds_count;
        panel_config.vendor_config = &vendor;
        ESP_RETURN_ON_ERROR(esp_lcd_new_panel_nv3041(g_panellan_lcd_rt.io, &panel_config, &g_panellan_lcd_rt.panel),
                            TAG, "nv3041");
        break;
    }
#endif
#if CONFIG_PANELLAN_LCD_PANEL_ST7789
    case PANELLAN_LCD_PANEL_ST7789: {
        static st7789_vendor_config_t vendor;
        vendor.init_cmds = (const st7789_lcd_init_cmd_t *)panel->vendor_init_cmds;
        vendor.init_cmds_size = (uint16_t)panel->vendor_init_cmds_count;
        ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(g_panellan_lcd_rt.io, &panel_config, &g_panellan_lcd_rt.panel),
                            TAG, "st7789");
        ESP_RETURN_ON_ERROR(panel_st7789_init(g_panellan_lcd_rt.io, &vendor), TAG, "st7789 vendor");
        break;
    }
#endif
    default:
        return ESP_ERR_NOT_SUPPORTED;
    }

    if (i80->call_panel_reset) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(g_panellan_lcd_rt.panel), TAG, "panel reset");
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(g_panellan_lcd_rt.panel), TAG, "panel init");
    ESP_RETURN_ON_ERROR(panellan_lcd_i80_clear_black(cfg), TAG, "startup black frame");
    ESP_RETURN_ON_ERROR(panellan_lcd_apply_orientation(g_panellan_lcd_rt.panel, panel, rotation), TAG, "orient");
    ESP_RETURN_ON_ERROR(panellan_lcd_panel_disp_on(panel), TAG, "disp_on");
    ESP_RETURN_ON_ERROR(panellan_lcd_panel_bl_on(panel), TAG, "bl_on");
    return ESP_OK;
}

esp_err_t panellan_lcd_deinit_i80(void) {
    if (g_panellan_lcd_rt.panel) {
        esp_lcd_panel_disp_on_off(g_panellan_lcd_rt.panel, false);
        esp_lcd_panel_del(g_panellan_lcd_rt.panel);
        g_panellan_lcd_rt.panel = NULL;
    }
    if (g_panellan_lcd_rt.io) {
        esp_lcd_panel_io_del(g_panellan_lcd_rt.io);
        g_panellan_lcd_rt.io = NULL;
    }
    if (g_panellan_lcd_rt.i80_bus) {
        esp_lcd_del_i80_bus(g_panellan_lcd_rt.i80_bus);
        g_panellan_lcd_rt.i80_bus = NULL;
    }
    panellan_lcd_i80_flush_cleanup();
    return ESP_OK;
}

void *panellan_lcd_i80_alloc_draw_buffer(size_t size, uint32_t caps) {
    esp_lcd_panel_io_handle_t io = panellan_lcd_get_io();
    if (!io) {
        return NULL;
    }
    return esp_lcd_i80_alloc_draw_buffer(io, size, caps);
}

#endif /* CONFIG_PANELLAN_LCD_ENABLE_I80 */
