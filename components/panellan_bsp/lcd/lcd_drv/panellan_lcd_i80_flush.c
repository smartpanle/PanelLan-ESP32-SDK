/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * I80 LVGL flush with SRAM bounce (avoid GDMA reading PSRAM under high PCLK):
 *
 *   LVGL draw buffer (PSRAM, PARTIAL)
 *        │  memcpy (CPU)
 *        ▼
 *   bounce[0|1] in internal DMA SRAM  ──ping-pong──► esp_lcd_panel_draw_bitmap
 *        │                                              │
 *        │                                              ▼ GDMA → I80 panel
 *        └──────── wait color_trans_done ───────────────┘
 *
 * Byte swap: panel_io flags.swap_color_bytes (HW), not adapter soft rgb565_swap.
 * Custom flush_cb replaces the adapter default so DMA always sources SRAM.
 *
 * max_transfer_bytes must be ≥ bounce half size (see board panellan_bsp_config).
 */

#include "sdkconfig.h"

#if CONFIG_PANELLAN_LCD_ENABLE_I80

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_lcd_io_i80.h"
#include "esp_lcd_panel_ops.h"

#include "lvgl.h"

#include "panellan_lcd.h"

static const char *TAG = "panellan_lcd_i80";

/** One bounce bank size; each draw_bitmap chunk must fit here. */
#define PANELLAN_LCD_I80_BOUNCE_HALF_DEFAULT  (16 * 1024)
/** Prefer chunk sizes multiple of DMA burst (board dma_burst_size is typically 64). */
#define PANELLAN_LCD_I80_BOUNCE_ALIGN         (64)

static SemaphoreHandle_t s_i80_color_done_sem;
static uint8_t *s_i80_bounce[2];
static size_t s_i80_bounce_half_bytes;

/** ISR: one color DMA finished → unblock flush task waiting on the semaphore. */
static bool IRAM_ATTR panellan_lcd_i80_on_color_done(esp_lcd_panel_io_handle_t panel_io,
                                                 esp_lcd_panel_io_event_data_t *edata,
                                                 void *user_ctx) {
    (void)panel_io;
    (void)edata;
    (void)user_ctx;
    BaseType_t need_yield = pdFALSE;
    if (s_i80_color_done_sem) {
        xSemaphoreGiveFromISR(s_i80_color_done_sem, &need_yield);
    }
    return need_yield == pdTRUE;
}

/** Drop stale done tokens so the first chunk of a new flush does not take early. */
static void panellan_lcd_i80_drain_color_sem(void) {
    if (!s_i80_color_done_sem) {
        return;
    }
    while (xSemaphoreTake(s_i80_color_done_sem, 0) == pdTRUE) {
    }
}

/**
 * How many RGB565 rows fit in one bounce bank for this dirty width.
 * Prefer (rows * line_bytes) % ALIGN == 0; if impossible for this width, still
 * return max rows that fit (never fall back to a full-frame draw_bitmap).
 */
static size_t panellan_lcd_i80_rows_per_chunk(size_t line_bytes) {
    if (line_bytes == 0 || line_bytes > s_i80_bounce_half_bytes) {
        return 0;
    }
    size_t rows = s_i80_bounce_half_bytes / line_bytes;
    size_t aligned = rows;
    while (aligned > 0 && ((aligned * line_bytes) % PANELLAN_LCD_I80_BOUNCE_ALIGN) != 0) {
        aligned--;
    }
    return aligned > 0 ? aligned : rows;
}

/**
 * Copy dirty area from PSRAM color_map into bounce banks and issue draw_bitmap
 * per chunk. Ping-pong: while GDMA drains bank N, CPU fills bank N^1, then wait
 * for the previous transfer before starting the next (overlap memcpy + DMA).
 */
static esp_err_t panellan_lcd_i80_custom_draw_bitmap(lv_display_t *disp,
                                                 esp_lcd_panel_handle_t panel,
                                                 int x_start, int y_start,
                                                 int x_end, int y_end,
                                                 const void *color_map) {
    if (!disp || !panel || !color_map || !s_i80_color_done_sem ||
            !s_i80_bounce[0] || !s_i80_bounce[1]) {
        return ESP_ERR_INVALID_ARG;
    }

    const int width = x_end - x_start;
    const int height = y_end - y_start;
    if (width <= 0 || height <= 0) {
        lv_display_flush_ready(disp);
        return ESP_OK;
    }

    const size_t line_bytes = (size_t)width * sizeof(uint16_t);
    size_t rows_per_chunk = panellan_lcd_i80_rows_per_chunk(line_bytes);
    if (rows_per_chunk == 0) {
        /* One line larger than bounce — cannot chunk safely within max_transfer. */
        ESP_LOGE(TAG, "bounce too small for line %dx%d (%u B)", width, height,
                 (unsigned)line_bytes);
        return ESP_ERR_INVALID_SIZE;
    }

    const uint8_t *src = (const uint8_t *)color_map;
    int y = y_start;
    int remaining = height;
    int chunk_idx = 0;
    panellan_lcd_i80_drain_color_sem();

    while (remaining > 0) {
        const int rows = (remaining > (int)rows_per_chunk) ? (int)rows_per_chunk : remaining;
        const size_t chunk_bytes = (size_t)rows * line_bytes;
        uint8_t *dst = s_i80_bounce[chunk_idx & 1];

        memcpy(dst, src, chunk_bytes);

        /* First chunk: fire DMA immediately. Later: wait until previous DMA done
         * so we do not overwrite a bank still being read by GDMA. */
        if (chunk_idx > 0) {
            if (xSemaphoreTake(s_i80_color_done_sem, portMAX_DELAY) != pdTRUE) {
                return ESP_FAIL;
            }
        }

        esp_err_t ret = esp_lcd_panel_draw_bitmap(panel, x_start, y, x_end, y + rows, dst);
        if (ret != ESP_OK) {
            return ret;
        }

        src += chunk_bytes;
        y += rows;
        remaining -= rows;
        chunk_idx++;
    }

    /* Last chunk must finish before LVGL may reuse the PSRAM draw buffer. */
    if (xSemaphoreTake(s_i80_color_done_sem, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }

    lv_display_flush_ready(disp);
    return ESP_OK;
}

/** LVGL flush_cb: area is inclusive; esp_lcd wants exclusive x_end/y_end. */
static void panellan_lcd_i80_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *color_map) {
    esp_lcd_panel_handle_t panel = panellan_lcd_get_panel();
    if (!disp || !area || !color_map || !panel) {
        if (disp) {
            lv_display_flush_ready(disp);
        }
        return;
    }
    esp_err_t ret = panellan_lcd_i80_custom_draw_bitmap(disp, panel,
                                                    area->x1, area->y1,
                                                    area->x2 + 1, area->y2 + 1,
                                                    color_map);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I80 flush failed: %s", esp_err_to_name(ret));
        lv_display_flush_ready(disp);
    }
}

void panellan_lcd_i80_flush_cleanup(void) {
    if (s_i80_bounce[0]) {
        free(s_i80_bounce[0]);
        s_i80_bounce[0] = NULL;
    }
    if (s_i80_bounce[1]) {
        free(s_i80_bounce[1]);
        s_i80_bounce[1] = NULL;
    }
    if (s_i80_color_done_sem) {
        vSemaphoreDelete(s_i80_color_done_sem);
        s_i80_color_done_sem = NULL;
    }
    s_i80_bounce_half_bytes = 0;
}

/**
 * Install color-done callback, allocate bounce banks, set LVGL flush_cb.
 * Call after register_display and before esp_lv_adapter_start().
 */
esp_err_t panellan_lcd_i80_setup_adapter_flush(void *disp_v, esp_lcd_panel_io_handle_t io) {
    lv_display_t *disp = (lv_display_t *)disp_v;
    esp_lcd_panel_handle_t panel = panellan_lcd_get_panel();
    if (!disp || !io || !panel) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_i80_color_done_sem) {
        s_i80_color_done_sem = xSemaphoreCreateBinary();
        if (!s_i80_color_done_sem) {
            return ESP_ERR_NO_MEM;
        }
    }

    if (!s_i80_bounce[0] || !s_i80_bounce[1]) {
        s_i80_bounce_half_bytes = PANELLAN_LCD_I80_BOUNCE_HALF_DEFAULT;
        s_i80_bounce[0] = esp_lcd_i80_alloc_draw_buffer(io, s_i80_bounce_half_bytes,
                                                        MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        s_i80_bounce[1] = esp_lcd_i80_alloc_draw_buffer(io, s_i80_bounce_half_bytes,
                                                        MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        if (!s_i80_bounce[0] || !s_i80_bounce[1]) {
            ESP_LOGE(TAG, "alloc I80 bounce failed (half=%u)", (unsigned)s_i80_bounce_half_bytes);
            return ESP_ERR_NO_MEM;
        }
        ESP_LOGI(TAG, "I80 SRAM bounce ping-pong half=%u @%p / %p",
                 (unsigned)s_i80_bounce_half_bytes, s_i80_bounce[0], s_i80_bounce[1]);
    }

    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = panellan_lcd_i80_on_color_done,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_register_event_callbacks(io, &cbs, disp), TAG,
                        "register I80 color-done failed");

    lv_display_set_flush_cb(disp, panellan_lcd_i80_flush_cb);
    ESP_LOGI(TAG, "I80 bounce + HW swap_color_bytes (custom flush, no soft rgb565_swap)");
    return ESP_OK;
}

/**
 * Bind LVGL PARTIAL double buffers in PSRAM (full frame size each).
 * Bounce SRAM is separate and allocated in setup_adapter_flush().
 */
esp_err_t panellan_lcd_i80_bind_lvgl_draw_buffer(void *disp_v) {
    lv_display_t *disp = (lv_display_t *)disp_v;
    esp_lcd_panel_io_handle_t io = panellan_lcd_get_io();
    if (!disp || !io) {
        return ESP_ERR_INVALID_ARG;
    }

    const size_t draw_buf_bytes =
        (size_t)panellan_lcd_get_h_res() * (size_t)panellan_lcd_get_v_res() * sizeof(uint16_t);

    void *buf1 = esp_lcd_i80_alloc_draw_buffer(io, draw_buf_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    void *buf2 = esp_lcd_i80_alloc_draw_buffer(io, draw_buf_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    if (!buf1 || !buf2) {
        ESP_LOGE(TAG, "PSRAM I80 draw buffer alloc failed sz=%u", (unsigned)draw_buf_bytes);
        return ESP_ERR_NO_MEM;
    }
    lv_display_set_buffers(disp, buf1, buf2, draw_buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    ESP_LOGI(TAG, "I80 PSRAM draw buf partial sz=%u", (unsigned)draw_buf_bytes);
    return ESP_OK;
}

#endif /* CONFIG_PANELLAN_LCD_ENABLE_I80 */
