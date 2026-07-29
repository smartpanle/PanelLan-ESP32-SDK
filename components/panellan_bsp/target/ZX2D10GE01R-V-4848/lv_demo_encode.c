/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_log.h"
#include "esp_lv_adapter.h"

#include "iot_button.h"

#include "lv_demo_encode.h"
#include "panellan_bsp.h"

static const char *TAG = "ENC_DEMO";

/* Button state (set by iot_button callbacks, synced to UI by LVGL timer) */
static bool      s_btn_pressed   = false;

/* LVGL widgets (set by UI init, read by timer callback) */
static lv_obj_t *s_btn          = NULL;
static lv_obj_t *s_btn_label    = NULL;
static bool      s_btn_last     = false;

static void btn_down_cb(void *arg, void *arg2) {
    (void)arg; (void)arg2;
    s_btn_pressed = true;
    ESP_LOGD(TAG, "BTN DOWN");
}

static void btn_up_cb(void *arg, void *arg2) {
    (void)arg; (void)arg2;
    s_btn_pressed = false;
    ESP_LOGD(TAG, "BTN UP");
}

static void btn_ui_sync_timer(lv_timer_t *t) {
    (void)t;
    if (!s_btn || !s_btn_label) return;

    if (s_btn_pressed != s_btn_last) {
        s_btn_last = s_btn_pressed;
        if (s_btn_pressed) {
            lv_label_set_text(s_btn_label, LV_SYMBOL_OK " PRESSED");
            lv_obj_add_state(s_btn, LV_STATE_PRESSED);
        } else {
            lv_label_set_text(s_btn_label, LV_SYMBOL_SETTINGS " ENTER");
            lv_obj_clear_state(s_btn, LV_STATE_PRESSED);
        }
    }
}

// Arc value change callback
static void arc_value_changed_cb(lv_event_t *e) {
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_t *arc   = lv_event_get_target_obj(e);
    int32_t val     = lv_arc_get_value(arc);
    lv_label_set_text_fmt(label, "%" PRId32 " deg", val);
}

// Public API
lv_indev_t *lv_demo_encode_indev_init(lv_display_t *disp) {
    ESP_LOGI(TAG, "Initializing encoder input via esp_lv_adapter");

    // init
    esp_lv_adapter_encoder_config_t encoder_config = {
        .disp         = disp,
        .encoder_a_b  = hw_knob_get_config(),
        .encoder_enter = hw_knob_get_button(),
    };

    lv_indev_t *encoder = esp_lv_adapter_register_encoder(&encoder_config);
    if (encoder == NULL) {
        ESP_LOGE(TAG, "esp_lv_adapter_register_encoder failed");
        return NULL;
    }

    lv_indev_set_mode(encoder, LV_INDEV_MODE_TIMER);
    ESP_LOGI(TAG, "Encoder indev ready (adapter + TIMER mode fix)");

    button_handle_t btn = hw_knob_get_button();
    if (btn) {
        iot_button_register_cb(btn, BUTTON_PRESS_DOWN, NULL, btn_down_cb, NULL);
        iot_button_register_cb(btn, BUTTON_PRESS_UP,   NULL, btn_up_cb,   NULL);
        ESP_LOGI(TAG, "Button UI sync callbacks registered");
    }

    return encoder;
}

void lv_demo_encode_ui(void) {
    ESP_LOGI(TAG, "Creating encoder demo UI");

    lv_indev_t *enc = lv_indev_get_next(NULL);
    while (enc) {
        if (lv_indev_get_type(enc) == LV_INDEV_TYPE_ENCODER) break;
        enc = lv_indev_get_next(enc);
    }
    if (!enc) {
        ESP_LOGE(TAG, "No encoder indev found — call lv_demo_encode_indev_init() first");
        return;
    }

    lv_group_t *group = lv_group_create();
    lv_indev_set_group(enc, group);
    lv_group_set_editing(group, true);   /* enc_diff → direct value adjust */

    lv_obj_t *arc = lv_arc_create(lv_screen_active());
    lv_obj_set_size(arc, 200, 200);
    lv_obj_center(arc);
    lv_arc_set_range(arc, 0, 360);
    lv_arc_set_value(arc, 180);
    lv_arc_set_rotation(arc, 135);
    lv_group_add_obj(group, arc);

    lv_obj_t *arc_label = lv_label_create(lv_screen_active());
    lv_label_set_text(arc_label, "180 deg");
    lv_obj_align_to(arc_label, arc, LV_ALIGN_BOTTOM_MID, 0, 15);
    lv_obj_add_event_cb(arc, arc_value_changed_cb, LV_EVENT_VALUE_CHANGED, arc_label);

    s_btn = lv_button_create(lv_screen_active());
    lv_obj_set_size(s_btn, 120, 50);
    lv_obj_align(s_btn, LV_ALIGN_BOTTOM_MID, 0, -60);

    s_btn_label = lv_label_create(s_btn);
    lv_label_set_text(s_btn_label, LV_SYMBOL_SETTINGS " ENTER");
    lv_obj_center(s_btn_label);

    lv_obj_t *title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "Encoder Demo");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_timer_create(btn_ui_sync_timer, 50, NULL);

    ESP_LOGI(TAG, "Encoder demo UI ready");
}
