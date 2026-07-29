/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_indev_t *lv_demo_encode_indev_init(lv_display_t *disp);
void lv_demo_encode_ui(void);

#ifdef __cplusplus
}
#endif
