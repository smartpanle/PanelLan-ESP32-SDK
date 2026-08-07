# PanelLan-Starter

[中文](README_CN.md) | English

Multi-board ESP32 display-module BSP. The unified component `components/panellan_bsp` owns LCD / touch / encoder; the app calls `hw_lcd_init` / `hw_touch_init`, and UI runs on `esp_lvgl_adapter` + LVGL.

**Principle:** Prefer ESP-IDF / Component Registry drivers; this repo only assembles buses and encodes board differences (pins, timing, vendor tables).

---

## 1. Versions & environment

| Item | Requirement | Notes |
|------|-------------|--------|
| **ESP-IDF** | **≥ 5.5** | Effective floor from `esp_lvgl_adapter` (`idf >= 5.5`); component yml still lists `idf >= 5.3` |
| **LVGL** | **`>=8,<10`** | Resolved by Component Manager; usually **v9.x** |
| **esp_lvgl_adapter** | **^0.6.2** | In `components/panellan_bsp/idf_component.yml` |

**Python (`idf.py` / Component Manager):** Match your ESP-IDF install (often **3.10.x**). With [pyenv](https://github.com/pyenv/pyenv), pick a version for the **current terminal only** (does not change global default):

```bash
pyenv shell 3.10.14
```

Then `source $IDF_PATH/export.sh` (or your IDF env script) before running `idf.py`.

---

## 2. Quick start (this repository)

```bash
cd /path/to/PanelLan-Starter
pyenv shell 3.10.14    # optional: Python 3.10.14 for this shell only
rm -f sdkconfig && rm -rf build
idf.py -DBOARD=ZX3D50CE02S-USRC-4832 set-target esp32s3 build
idf.py -p /dev/ttyUSB0 flash monitor     # Windows: -p COM3
```

After `BOARD` is cached, use `idf.py build` / `flash monitor` on the same board. Exit monitor: `Ctrl+]`.

Boot path: `main/main.c` → `hw_lcd_init` → adapter display register → (I80: PSRAM draw buf + SRAM bounce flush) → optional touch → LVGL demo.

---

## 3. ★ Use in your own project

Copy `components/panellan_bsp` into your ESP-IDF app, then wire it as below.

### 3.1 Copy the component

```bash
cp -a /path/to/PanelLan-Starter/components/panellan_bsp \
      /path/to/your_project/components/
```

Or point `override_path` at that directory from `main/idf_component.yml` (see below).

### 3.2 Root `CMakeLists.txt` (before `project.cmake`)

Board-select logic lives in the component. At minimum `include` it **before** `project.cmake`. If you use LVGL / `esp_lvgl_adapter`, keep the same LVGL heap macros as this repo:

```cmake
cmake_minimum_required(VERSION 3.16)

include(${CMAKE_CURRENT_LIST_DIR}/components/panellan_bsp/cmake/board_select.cmake)

add_definitions(-DLV_MEM_CUSTOM=1)
add_definitions(-DCONFIG_LV_MEM_POOL_ALLOC=malloc)
add_definitions(-DCONFIG_LV_MEM_POOL_INCLUDE=<stdlib.h>)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(your_app)
```

### 3.3 `main` dependency and API

Example `main/idf_component.yml`:

```yaml
dependencies:
  panellan/panellan_bsp:
    version: "*"
    override_path: "../components/panellan_bsp"
```

App code:

```c
#include "panellan_bsp.h"

hw_lcd_init(...);
/* if touch: */
hw_touch_init(...);
```

See this repo’s `main/main.c` for `esp_lvgl_adapter` / I80 bounce flush.

Also place **`partitions.csv`** at the project root (must fit the board flash size; this repo uses ~6M factory, board defaults are typically 8MB/16MB). Common `sdkconfig.defaults` should include:

```text
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
```

### 3.4 Select board and build

```bash
rm -f sdkconfig && rm -rf build
idf.py -DBOARD=ZX3D50CE02S-USRC-4832 set-target esp32s3 build
idf.py -p PORT flash monitor
```

`-DBOARD=` is the **full directory name** under `panellan_bsp/target/`.

Notes:

- **First configure / board change requires `-DBOARD=`**; otherwise flash often stays at 2MB and clashes with the partition table.
- **Do not** switch boards only by flipping `CONFIG_ZX*` in menuconfig; clear `sdkconfig` + `build` and pass `-DBOARD=` again.
- Same-board rebuilds may omit `-DBOARD=` (value is in CMakeCache).
- `esp_lvgl_adapter` rewrites `managed_components/lvgl__lvgl/.../lv_freertos.c` at configure time; if `idf.py fullclean` complains about a hash mismatch, run `rm -rf managed_components/lvgl__lvgl` first.

Board list: [§4](#4-supported-boards).

---

## 4. Supported boards

`BOARD` must be the **full directory name** under `components/panellan_bsp/target/`:

| `BOARD` | Resolution | Input | Bus / panel |
|---------|------------|-------|-------------|
| `ZX2D10GE01R-V-4848` | 480×480 | Encoder | RGB + SPI9 |
| `ZX2D80CE02S-2432` | 240×320 | Touch | I80 + ST7789 |
| `ZX2D80CE63S-2432` | 240×320 | Touch | SPI + ST7789 |
| `ZX2D80CECOF-2432` | 240×320 | Touch | I80 + ST7789 |
| `ZX3D50CE02S-USRC-4832` | 320×480 | Touch | I80 + ST7796 |
| `ZX3D95CE01S-TR-4848` | 480×480 | Touch | RGB + SPI9 |
| `ZX4D30CE08S-4827` | 480×272 | Touch | I80 + NV3041 |
| `ZX4D30NE01S-UR-4827` | 480×272 | None | RGB |
| `ZX7D00CE01S-8048` | 800×480 | Touch | RGB + GT911 |

Partition table: root `partitions.csv`. Panel / touch / encoder deps are pulled by Kconfig rules.

Architecture and how to add boards / drivers: [`components/panellan_bsp/README.md`](components/panellan_bsp/README.md).

---

## 5. Command cheat sheet

```bash
rm -f sdkconfig && rm -rf build
idf.py -DBOARD=ZX3D50CE02S-USRC-4832 set-target esp32s3 build
idf.py -p PORT flash monitor

idf.py build          # rebuild same board
idf.py menuconfig
# if fullclean complains about lvgl hash:
rm -rf managed_components/lvgl__lvgl && idf.py fullclean
```
