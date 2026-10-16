# PanelLan-Starter

[English](README.md) | 中文

多板型 ESP32 显示模组 BSP 工程：统一组件 `components/panellan_bsp` 管理 LCD / 触摸 / 旋钮，应用通过 `hw_lcd_init` / `hw_touch_init` 接入，UI 走 `esp_lvgl_adapter` + LVGL。

**设计原则**：优先复用 ESP-IDF / Component Registry 原驱动；本仓只做总线组装与板级差异（引脚、时序、vendor 表）。

---

## 1. 版本与环境


| 项                    | 要求         | 说明                                                                             |
| -------------------- | ---------- | ------------------------------------------------------------------------------ |
| **ESP-IDF**          | **≥ 5.5**  | 实际以 `esp_lvgl_adapter` 为准（`idf >= 5.5`）；组件 yml 里 idf 写的是 `>=5.3`，会被 adapter 收紧 |
| **LVGL**             | `>=8,<10`  | Component Manager 解析；日常为 **v9.x**                                              |
| **esp_lvgl_adapter** | **^0.6.2** | 声明于 `components/panellan_bsp/idf_component.yml`                                  |

**Python（`idf.py` / Component Manager）**：需与当前 ESP-IDF 要求一致（常见为 3.10.x）。已安装 [pyenv](https://github.com/pyenv/pyenv) 时，可在**当前终端**临时指定版本（仅对本 shell 生效）：

```bash
pyenv shell 3.10.14
```

然后再 `source $IDF_PATH/export.sh`（或你的 IDF 环境脚本），再执行 `idf.py`。

---

## 2. 本仓库快速开始

```bash
cd /path/to/PanelLan-Starter
pyenv shell 3.10.14    # 可选：当前 shell 临时 Python 3.10.14
rm -f sdkconfig && rm -rf build
idf.py -DBOARD=ZX3D50CE02S-USRC-4832 set-target esp32s3 build
idf.py -p /dev/ttyUSB0 flash monitor     # Windows: -p COM3
```

`BOARD` 写入 CMakeCache 后，同板可直接 `idf.py build` / `flash monitor`。退出监视：`Ctrl+]`。

入口：`main/main.c` → `hw_lcd_init` → adapter 注册显示 →（I80：绑定 PSRAM 画缓冲 + SRAM bounce flush）→ 可选触摸 → LVGL demo。

---

## 3. ★ 自定义工程使用例程（重点）

把本仓的 `components/panellan_bsp` 拷进你的 ESP-IDF 工程后，按下面接入即可选板编译。

### 3.1 拷贝组件

```bash
cp -a /path/to/PanelLan-Starter/components/panellan_bsp \
      /path/to/your_project/components/
```

或在 `main/idf_component.yml` 里用 `override_path` 指向该目录（本仓写法见下）。

### 3.2 根 `CMakeLists.txt`（必须在 `project.cmake` 之前）

选板逻辑在组件内；根工程至少 `include` 一行。若跑 LVGL / `esp_lvgl_adapter`，建议一并加上本仓的 LVGL 内存宏：

```cmake
cmake_minimum_required(VERSION 3.16)

include(${CMAKE_CURRENT_LIST_DIR}/components/panellan_bsp/cmake/board_select.cmake)

add_definitions(-DLV_MEM_CUSTOM=1)
add_definitions(-DCONFIG_LV_MEM_POOL_ALLOC=malloc)
add_definitions(-DCONFIG_LV_MEM_POOL_INCLUDE=<stdlib.h>)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(your_app)
```



### 3.3 `main` 依赖与代码

`main/idf_component.yml` 示例：

```yaml
dependencies:
  panellan/panellan_bsp:
    version: "*"
    override_path: "../components/panellan_bsp"
```

应用侧：

```c
#include "panellan_bsp.h"

hw_lcd_init(...);
/* 有触摸再： */
hw_touch_init(...);
```

可参考本仓 `main/main.c`（含 `esp_lvgl_adapter` / I80 bounce flush）。

工程根还需准备 `partitions.csv`（与板级 Flash 大小匹配；本仓 factory 约 6M，板 defaults 多为 8MB/16MB）。公共 `sdkconfig.defaults` 里应有：

```text
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
```



### 3.4 选板编译

```bash
rm -f sdkconfig && rm -rf build
idf.py -DBOARD=ZX3D50CE02S-USRC-4832 set-target esp32s3 build
idf.py -p PORT flash monitor
```

`-DBOARD=` 为 `panellan_bsp/target/` 下**目录全名**。

注意：

- **首次 / 换板必须带 `-DBOARD=`**，否则 Flash 常停在 2MB，易与分区表冲突。
- **不要**只靠 menuconfig 改 `CONFIG_ZX*` 换板；换板请清 `sdkconfig` + `build` 后重新带 `-DBOARD=`。
- 同板再次编译可省略 `-DBOARD=`（已写入 CMakeCache）。
- `esp_lvgl_adapter` 配置时会改写 `managed_components/lvgl__lvgl/.../lv_freertos.c`；若 `idf.py fullclean` 报哈希不匹配，先 `rm -rf managed_components/lvgl__lvgl` 再清理。

板一览见 [§4](#4-支持板型)。

---

## 4. 支持板型

`BOARD` 必须用 `components/panellan_bsp/target/` 下**目录全名**：


| `BOARD`                 | 分辨率     | 输入  | 接口           |
| ----------------------- | ------- | --- | ------------ |
| `ZX2D10GE01R-V-4848`    | 480×480 | 编码器 | RGB + SPI9   |
| `ZX2D80CE02S-2432`      | 240×320 | 触摸  | I80 + ST7789 |
| `ZX2D80CE63S-2432`      | 240×320 | 触摸  | SPI + ST7789 |
| `ZX2D80CECOF-2432`      | 240×320 | 触摸  | I80 + ST7789 |
| `ZX3D50CE02S-USRC-4832` | 320×480 | 触摸  | I80 + ST7796 |
| `ZX3D95CE01S-TR-4848`   | 480×480 | 触摸  | RGB + SPI9   |
| `ZX4D30CE08S-4827`      | 480×272 | 触摸  | I80 + NV3041 |
| `ZX4D30NE01S-UR-4827`   | 480×272 | 无   | RGB          |
| `ZX7D00CE01S-8048`      | 800×480 | 触摸  | RGB + GT911  |


分区表：根目录 `partitions.csv`。屏 / 触摸 / 旋钮等依赖按 Kconfig 条件拉取。

分层架构、加板 / 加驱步骤见组件文档：`[components/panellan_bsp/README_CN.md](components/panellan_bsp/README_CN.md)`。

---

## 5. 命令速查

```bash
rm -f sdkconfig && rm -rf build
idf.py -DBOARD=ZX3D50CE02S-USRC-4832 set-target esp32s3 build
idf.py -p PORT flash monitor

idf.py build          # 同板再编
idf.py menuconfig
# fullclean 若报 lvgl 哈希不匹配：
rm -rf managed_components/lvgl__lvgl && idf.py fullclean
```

### ZX3D50CE02S-USRC-4832 触摸芯片选择

此型号支持 FT5x06（默认）和 GT1151，两种 LCD 屏幕版本均使用所选触摸芯片。
运行 `idf.py menuconfig`，进入 `PanelLan config → Touch → FT5x06 / GT1151 touch IC`，
按实际硬件选择芯片，再执行 `idf.py reconfigure build`。
GT1151 使用乐鑫组件库 `espressif/esp_lcd_touch_gt1151`，固定版本为 `1.1.1`。
此型号的选择项自动启用对应驱动，并关闭未选中的 FT5x06 / GT1151 驱动。
该通用选择适用于所有 MCU；选择 None 可关闭这两种驱动，使用 GT911、CST3240 或无触摸配置。

通过配置文件选择 GT1151 时，设置 `CONFIG_PANELLAN_TOUCH_GT1151=y`，
并取消 `CONFIG_PANELLAN_TOUCH_FT5X06`。原有 SDA GPIO6、SCL GPIO5、INT GPIO7
以及板级复位方式保持不变；GT1151 使用官方默认 I2C 地址 `0x14`。
接入硬件后需确认 I2C 通信、触摸坐标和各旋转方向。
