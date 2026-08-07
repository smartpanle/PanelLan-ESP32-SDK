/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lv_adapter.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "panellan_bsp.h"

#include "lv_demos.h"
#if HW_USE_ENCODER && CONFIG_ESP_LVGL_ADAPTER_ENABLE_KNOB
#include "lv_demo_encode.h"
#endif

static const char *TAG = "main";

#define FPS_MONITOR_TASK_STACK_SIZE 4096
#define FPS_MONITOR_TASK_PRIORITY 3
#define FPS_MONITOR_INTERVAL_MS 1000

#if HW_USE_LCD
static panellan_display_rotation_t get_configured_rotation(void) {
#if CONFIG_EXAMPLE_DISPLAY_ROTATION_0
    return PANELLAN_DISPLAY_ROTATE_0;
#elif CONFIG_EXAMPLE_DISPLAY_ROTATION_90
    return PANELLAN_DISPLAY_ROTATE_90;
#elif CONFIG_EXAMPLE_DISPLAY_ROTATION_180
    return PANELLAN_DISPLAY_ROTATE_180;
#elif CONFIG_EXAMPLE_DISPLAY_ROTATION_270
    return PANELLAN_DISPLAY_ROTATE_270;
#else
    return PANELLAN_DISPLAY_ROTATE_0;
#endif
}

static esp_lv_adapter_rotation_t to_adapter_rotation(panellan_display_rotation_t rotation) {
    switch (rotation) {
    case PANELLAN_DISPLAY_ROTATE_0:
        return (esp_lv_adapter_rotation_t)0;
    case PANELLAN_DISPLAY_ROTATE_90:
        return (esp_lv_adapter_rotation_t)90;
    case PANELLAN_DISPLAY_ROTATE_180:
        return (esp_lv_adapter_rotation_t)180;
    case PANELLAN_DISPLAY_ROTATE_270:
        return (esp_lv_adapter_rotation_t)270;
    default:
        return (esp_lv_adapter_rotation_t)0;
    }
}
#endif

#if HW_USE_LCD && CONFIG_ESP_LVGL_ADAPTER_ENABLE_FPS_STATS
static void fps_monitor_task(void *arg) {
    lv_display_t *disp = (lv_display_t *)arg;
    uint32_t fps;
    while (1) {
        if (esp_lv_adapter_get_fps(disp, &fps) == ESP_OK) {
            ESP_LOGI(TAG, "Current FPS: %lu", fps);
        }
        vTaskDelay(pdMS_TO_TICKS(FPS_MONITOR_INTERVAL_MS));
    }
}
#endif

#if HW_USE_LCD
static void app_init_display(void) {
    esp_lcd_panel_handle_t display_panel = NULL;
    esp_lcd_panel_io_handle_t display_io_handle = NULL;
    panellan_display_rotation_t rotation = get_configured_rotation();
    esp_lv_adapter_rotation_t adapter_rotation = to_adapter_rotation(rotation);
    esp_lv_adapter_tear_avoid_mode_t tear_avoid_mode;
    esp_lv_adapter_display_config_t display_config;

    /* Tear-avoid / display profile by LCD bus type (not by board name) */
#if CONFIG_PANELLAN_LCD_ENABLE_MIPI_DSI
    tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DEFAULT_MIPI_DSI;
    ESP_LOGI(TAG, "LCD bus: MIPI DSI");
#elif CONFIG_PANELLAN_LCD_ENABLE_RGB
    tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DEFAULT_RGB;
    ESP_LOGI(TAG, "LCD bus: RGB");
#elif CONFIG_PANELLAN_LCD_ENABLE_I80
    tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE;
    ESP_LOGI(TAG, "LCD bus: I80");
#elif CONFIG_PANELLAN_LCD_ENABLE_QSPI
    tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DEFAULT;
    ESP_LOGI(TAG, "LCD bus: QSPI");
#elif CONFIG_PANELLAN_LCD_ENABLE_SPI_WITH_PSRAM || CONFIG_PANELLAN_LCD_ENABLE_SPI_WITHOUT_PSRAM
    {
        const int te_gpio = hw_lcd_get_te_gpio();
        if (te_gpio != GPIO_NUM_NC) {
            tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TE_SYNC;
            ESP_LOGI(TAG, "LCD bus: SPI (TE GPIO %d)", te_gpio);
        } else {
            tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DEFAULT;
            ESP_LOGI(TAG, "LCD bus: SPI");
        }
    }
#endif

    ESP_LOGI(TAG, "Initializing LCD: %dx%d", HW_LCD_H_RES, HW_LCD_V_RES);
    ESP_ERROR_CHECK(hw_lcd_init(&display_panel, &display_io_handle, (int)tear_avoid_mode, rotation));

    /* ---- LVGL adapter ---- */
    esp_lv_adapter_config_t adapter_config = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(esp_lv_adapter_init(&adapter_config));
    /* Do NOT start yet — I80 must install custom flush before LVGL worker runs (tmp_sdk order) */

#if CONFIG_PANELLAN_LCD_ENABLE_I80
    /* Must be before register_display: I80 owns color_done / flush_ready */
    ESP_ERROR_CHECK(esp_lv_adapter_set_default_display_idf_callback_registration_enabled(false));
#endif

#if CONFIG_PANELLAN_LCD_ENABLE_MIPI_DSI
    display_config = ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(
        display_panel, display_io_handle, HW_LCD_H_RES, HW_LCD_V_RES, adapter_rotation);
#elif CONFIG_PANELLAN_LCD_ENABLE_RGB
    display_config = ESP_LV_ADAPTER_DISPLAY_RGB_DEFAULT_CONFIG(
        display_panel, display_io_handle, HW_LCD_H_RES, HW_LCD_V_RES, adapter_rotation);
#elif CONFIG_PANELLAN_LCD_ENABLE_I80
    /* I80: PSRAM partial + SRAM bounce (tmp_sdk path) */
    display_config = (esp_lv_adapter_display_config_t){
        .panel = display_panel,
        .panel_io = display_io_handle,
        .profile = {
            .interface = ESP_LV_ADAPTER_PANEL_IF_OTHER,
            .rotation = adapter_rotation,
            .hor_res = HW_LCD_H_RES,
            .ver_res = HW_LCD_V_RES,
            .buffer_height = HW_LCD_V_RES,
            .use_psram = true,
            .enable_ppa_accel = false,
            .require_double_buffer = true,
            .mono_layout = ESP_LV_ADAPTER_MONO_LAYOUT_NONE,
        },
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE,
        .te_sync = ESP_LV_ADAPTER_TE_SYNC_DISABLED(),
    };
#elif CONFIG_PANELLAN_LCD_ENABLE_SPI_WITHOUT_PSRAM
    display_config = ESP_LV_ADAPTER_DISPLAY_SPI_WITHOUT_PSRAM_DEFAULT_CONFIG(
        display_panel, display_io_handle, HW_LCD_H_RES, HW_LCD_V_RES, adapter_rotation);
#elif CONFIG_PANELLAN_LCD_ENABLE_SPI_WITH_PSRAM
    {
        const int te_gpio = hw_lcd_get_te_gpio();
        if (te_gpio != GPIO_NUM_NC) {
            display_config = ESP_LV_ADAPTER_DISPLAY_SPI_WITH_PSRAM_TE_DEFAULT_CONFIG(
                display_panel, display_io_handle,
                HW_LCD_H_RES, HW_LCD_V_RES, adapter_rotation,
                te_gpio,
                hw_lcd_get_bus_freq_hz(),
                hw_lcd_get_bus_data_lines(),
                hw_lcd_get_bits_per_pixel());
        } else {
            display_config = ESP_LV_ADAPTER_DISPLAY_SPI_WITH_PSRAM_DEFAULT_CONFIG(
                display_panel, display_io_handle, HW_LCD_H_RES, HW_LCD_V_RES, adapter_rotation);
        }
    }
#elif CONFIG_PANELLAN_LCD_ENABLE_QSPI
    /* QSPI boards: reuse SPI-with-PSRAM default profile until a dedicated helper exists */
    display_config = ESP_LV_ADAPTER_DISPLAY_SPI_WITH_PSRAM_DEFAULT_CONFIG(
        display_panel, display_io_handle, HW_LCD_H_RES, HW_LCD_V_RES, adapter_rotation);
#endif

    lv_display_t *disp = esp_lv_adapter_register_display(&display_config);
    if (disp == NULL) {
        ESP_LOGE(TAG, "Failed to register display");
        return;
    }

#if CONFIG_PANELLAN_LCD_ENABLE_I80
    /* I80: PSRAM draw buffer + SRAM bounce flush — before adapter_start */
    if (hw_lcd_i80_bind_lvgl_draw_buffer(disp) != ESP_OK) {
        ESP_LOGE(TAG, "I80 PSRAM draw buffer bind failed");
        return;
    }
    if (hw_lcd_i80_setup_adapter_flush(disp, display_io_handle) != ESP_OK) {
        ESP_LOGE(TAG, "I80 bounce flush setup failed");
        return;
    }
    lv_display_flush_ready(disp); /* clear stale flushing before worker starts */
#endif

#if HW_USE_TOUCH
    ESP_LOGI(TAG, "Initializing touch panel");
    esp_lcd_touch_handle_t touch_handle = NULL;
    ESP_ERROR_CHECK(hw_touch_init(&touch_handle, rotation));

    esp_lv_adapter_touch_config_t touch_config = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, touch_handle);
    lv_indev_t *touch = esp_lv_adapter_register_touch(&touch_config);
    if (touch == NULL) {
        ESP_LOGE(TAG, "Failed to register touch");
        return;
    }
#elif HW_USE_ENCODER && CONFIG_ESP_LVGL_ADAPTER_ENABLE_KNOB
    /* Encoder input + LVGL indev (see lv_demo_encode.c) */
    if (lv_demo_encode_indev_init(disp) == NULL) {
        ESP_LOGE(TAG, "Encoder indev init failed");
        return;
    }
#endif

    ESP_ERROR_CHECK(esp_lv_adapter_start());

#if CONFIG_ESP_LVGL_ADAPTER_ENABLE_FPS_STATS
    ESP_ERROR_CHECK(esp_lv_adapter_fps_stats_enable(disp, true));
    xTaskCreate(fps_monitor_task, "fps_monitor", FPS_MONITOR_TASK_STACK_SIZE, disp, FPS_MONITOR_TASK_PRIORITY, NULL);
#endif

    hw_lcd_set_bl(true); /* re-enable backlight after LVGL takes over */

    /* Encoder demo UI (see lv_demo_encode.c) */
    if (esp_lv_adapter_lock(-1) == ESP_OK) {
#if HW_USE_ENCODER && CONFIG_ESP_LVGL_ADAPTER_ENABLE_KNOB
        lv_demo_encode_ui();
#else
        lv_demo_widgets();
        // lv_demo_benchmark();
#endif
        esp_lv_adapter_unlock();
    }
}

#endif /* HW_USE_LCD */

void app_main(void) {
#if HW_USE_LCD
    app_init_display();
#else
    ESP_LOGW(TAG, "No LCD bus enabled (PanelLan config → LCD); skipping display stack");
#if HW_USE_TOUCH
    ESP_LOGW(TAG, "Touch IC enabled but no LCD — skipping touch (needs a display for LVGL indev)");
#endif
#if HW_USE_ENCODER
    ESP_LOGW(TAG, "Encoder enabled but no LCD — skipping encoder indev/UI");
#endif
#endif

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
