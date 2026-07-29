# panellan_bsp

[中文](README_CN.md) | English

Unified PanelLan BSP component: **board (L3) + lcd/touch (L2)** over Espressif `esp_lcd` / `esp_lcd_touch` and Component Registry packages.

App API: `#include "panellan_bsp.h"` → `hw_lcd_init` / `hw_touch_init`.  
Project integration (copy component, `-DBOARD=`, partitions): see repo-root [README.md](../../README.md).

---

## Architecture

```text
App main / LVGL (esp_lvgl_adapter)
        │
        ▼
L3  Board   board/ + target/<BOARD>/     pins, timing, vendor, board defaults
        │
   ┌────┴────┐
   ▼         ▼
L2 LCD    L2 Touch     bus create / IC assemble / orientation / I80 bounce flush
   │         │
   └────┬────┘
        ▼
L1  esp_lcd / esp_lcd_touch / Registry (ST7796, FT5x06, GT911, …)
    Local only if missing upstream: lcd/panel/, touch/drivers/ (e.g. CST3240)
```

| Path | Layer | Role |
|------|-------|------|
| Registry `esp_lcd_*` / `esp_lcd_touch_*` | L1 | Upstream drivers |
| `lcd/`, `touch/` | L2 | Bus create + IC assemble |
| `board/`, `target/` | L3 | Pins, timing, vendor tables, board select |

Board select: `cmake/board_select.cmake` (included from the **project** root CMakeLists **before** `project.cmake`).

---

## Layout

```text
panellan_bsp/
  cmake/board_select.cmake
  include/          # panellan_bsp.h / panellan_lcd.h / panellan_touch.h
  board/            # L3 hw_* facade
  lcd/  touch/      # L2
  target/<BOARD>/   # L3 pins + g_panellan_bsp_config
  idf_component.yml
  Kconfig.projbuild
```

---

## Extending: boards / panels / touch

| Kind | Meaning | Where |
|------|---------|--------|
| **Board** | Pins, resolution, vendor table, default drivers | `target/<BOARD>/` |
| **Remote panel/touch** | Package already on Component Registry | `idf_component.yml` + Kconfig `rules` |
| **Private panel/touch** | Not in Registry / esp-bsp | `lcd/panel/`, `touch/drivers/` |

Prefer remote packages. Glass init **data** stays in board `panellan_bsp_config.c`. Helpers that only send init tables after an IDF panel (e.g. `lcd_drv/panel_st7789_vendor.c`) are **not** private panel drivers.

Also see `lcd/panel/README.md` and `touch/drivers/README.md`.

### Add a board

1. Copy the closest existing board (same bus / panel IC / touch):

| New board looks like | Prefer copy |
|----------------------|-------------|
| RGB, no touch | `ZX4D30NE01S-UR-4827` |
| RGB + FT5x06 | `ZX3D95CE01S-TR-4848` |
| I80 + ST7796 + FT | `ZX3D50CE02S-USRC-4832` |
| I80 + NV3041 + CST | `ZX4D30CE08S-4827` |
| Encoder | `ZX2D10GE01R-V-4848` |

```bash
cd target
cp -a <TEMPLATE> <NEW_BOARD_DIR>
```

2. Fill `target/<NEW_BOARD_DIR>/`:
   - `sdkconfig.defaults` — `CONFIG_ZX…=y`, resolution, `CONFIG_PANELLAN_LCD_ENABLE_*` / `PANEL_*` / `TOUCH_*`
   - `panellan_bsp_pin.h` — GPIO macros
   - `panellan_bsp_config.c` — export `g_panellan_bsp_config`
   - Optional: `panellan_bsp_prepare()` (weak empty by default)

3. Register (names must match defaults):
   - `Kconfig.projbuild` — `config ZX…` in `choice EXAMPLE_LCD_INTERFACE`
   - `CMakeLists.txt` — `elseif(CONFIG_ZX…)` → sources / include
   - Optionally `lcd/Kconfig.panellan` — H/V res `default if ZX…`
   - Update the board table in the **repo-root** README

4. Verify:

```bash
rm -f sdkconfig && rm -rf build
idf.py -DBOARD=<NEW_BOARD_DIR> set-target esp32s3 build
```

Expect `PanelLan board:` / `panellan_bsp: <dir>` and serial `LCD ready …`.

### Add a remote panel / touch

1. Add Registry dep in `idf_component.yml` with `rules` on `CONFIG_PANELLAN_*`.
2. Add Kconfig in `lcd/Kconfig.panellan` or `touch/Kconfig.panellan`.
3. Wire create/init in L2 (`lcd/lcd_drv/` or `touch/`).
4. Enable from board `sdkconfig.defaults` and fill pins/timing in `panellan_bsp_config.c`.

### Add a private panel / touch

Only when Registry / esp-bsp has no package (example: CST3240 under `touch/drivers/`):

1. Implement under `lcd/panel/` or `touch/drivers/`.
2. List sources in `CMakeLists.txt` behind the IC Kconfig.
3. Expose IC enum / init in L2 headers.
4. Boards select via config + defaults as above.
