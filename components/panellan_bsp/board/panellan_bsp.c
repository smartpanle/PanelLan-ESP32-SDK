/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panellan_bsp.h"
#if !CONFIG_ZX3D95CE01S_TR      && \
    !CONFIG_ZX3D50CE02S_USRC    && \
    !CONFIG_ZX4D30NE01S_UR      && \
    !CONFIG_ZX7D00CE01S         && \
    !CONFIG_ZX2D10GE01R         && \
    !CONFIG_ZX4D30CE08S         && \
    !CONFIG_ZX2D80CE02S         && \
    !CONFIG_ZX2D80CE63S
#error "No LCD interface selected! Please select one in menuconfig."
#endif
