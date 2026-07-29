/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Map touch rotation_type + display rotation to esp_lcd_touch flags
 * (swap_xy / mirror_x / mirror_y).
 */

#include "panellan_touch.h"

void panellan_touch_get_rotation_flags(panellan_touch_rotation_type_t type,
                                   panellan_display_rotation_t rotation,
                                   bool *swap_xy, bool *mirror_x, bool *mirror_y) {
    bool swap = false;
    bool x_mirror = false;
    bool y_mirror = false;

    if (type == PANELLAN_TOUCH_ROTATION_MIPI_DSI) {
        x_mirror = true;
        y_mirror = true;
        switch (rotation) {
        case PANELLAN_DISPLAY_ROTATE_90:
            swap = true;
            x_mirror = false;
            y_mirror = true;
            break;
        case PANELLAN_DISPLAY_ROTATE_180:
            swap = false;
            x_mirror = false;
            y_mirror = false;
            break;
        case PANELLAN_DISPLAY_ROTATE_270:
            swap = true;
            x_mirror = true;
            y_mirror = false;
            break;
        case PANELLAN_DISPLAY_ROTATE_0:
        default:
            swap = false;
            x_mirror = true;
            y_mirror = true;
            break;
        }
    } else if (type == PANELLAN_TOUCH_ROTATION_I80) {
        x_mirror = true;
        y_mirror = true;
        switch (rotation) {
        case PANELLAN_DISPLAY_ROTATE_90:
            swap = true;
            x_mirror = false;
            y_mirror = true;
            break;
        case PANELLAN_DISPLAY_ROTATE_180:
            swap = false;
            x_mirror = false;
            y_mirror = false;
            break;
        case PANELLAN_DISPLAY_ROTATE_270:
            swap = true;
            x_mirror = true;
            y_mirror = false;
            break;
        case PANELLAN_DISPLAY_ROTATE_0:
        default:
            swap = false;
            x_mirror = true;
            y_mirror = true;
            break;
        }
    } else {
        switch (rotation) {
        case PANELLAN_DISPLAY_ROTATE_90:
            swap = true;
            x_mirror = false;
            y_mirror = true;
            break;
        case PANELLAN_DISPLAY_ROTATE_180:
            swap = false;
            x_mirror = true;
            y_mirror = true;
            break;
        case PANELLAN_DISPLAY_ROTATE_270:
            swap = true;
            x_mirror = true;
            y_mirror = false;
            break;
        case PANELLAN_DISPLAY_ROTATE_0:
        default:
            swap = false;
            x_mirror = false;
            y_mirror = false;
            break;
        }
    }

    if (swap_xy) {
        *swap_xy = swap;
    }
    if (mirror_x) {
        *mirror_x = x_mirror;
    }
    if (mirror_y) {
        *mirror_y = y_mirror;
    }
}
