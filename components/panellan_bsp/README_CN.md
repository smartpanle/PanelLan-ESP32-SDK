# panellan_bsp

[English](README.md) | 中文

统一 PanelLan BSP 组件：**board（L3）+ lcd/touch（L2）**，底层对接乐鑫 `esp_lcd` / `esp_lcd_touch` 与 Component Registry 包。

应用 API：`#include "panellan_bsp.h"` → `hw_lcd_init` / `hw_touch_init`。  
工程接入（拷贝组件、`-DBOARD=`、分区表）见仓库根目录 [README_CN.md](../../README_CN.md)。

---

## 架构

```text
应用 main / LVGL (esp_lvgl_adapter)
        │
        ▼
L3  Board   board/ + target/<BOARD>/     引脚、时序、vendor、选板 defaults
        │
   ┌────┴────┐
   ▼         ▼
L2 LCD    L2 Touch     总线创建 / IC 组装 / 朝向 helper / I80 bounce flush
   │         │
   └────┬────┘
        ▼
L1  esp_lcd / esp_lcd_touch / Registry 包（ST7796、FT5x06、GT911…）
    无官方包时本地补：lcd/panel/、touch/drivers/（如 CST3240）
```

| 路径 | 层 | 作用 |
|------|----|------|
| Registry `esp_lcd_*` / `esp_lcd_touch_*` | L1 | 乐鑫 / 第三方原驱动 |
| `lcd/`、`touch/` | L2 | 总线创建与 IC 组装 |
| `board/`、`target/` | L3 | 引脚、时序、vendor 表、选板 |

选板：`cmake/board_select.cmake`（由**工程根** `CMakeLists.txt` 在 `project.cmake` **之前** `include`）。

---

## 目录

```text
panellan_bsp/
  cmake/board_select.cmake
  include/          # panellan_bsp.h / panellan_lcd.h / panellan_touch.h
  board/            # L3 hw_* 门面
  lcd/  touch/      # L2
  target/<BOARD>/   # L3 引脚 + g_panellan_bsp_config
  idf_component.yml
  Kconfig.projbuild
```

---

## 扩展：加板 / 加屏驱 / 加触摸

| 类型 | 是什么 | 放哪 |
|------|--------|------|
| **板型** | 引脚、分辨率、vendor 表、默认打开哪些驱动 | `target/<BOARD>/` |
| **远程屏驱 / 触摸** | Component Registry 已有包 | `idf_component.yml` + Kconfig `rules` |
| **私有屏驱 / 触摸** | Registry / esp-bsp **没有** | `lcd/panel/`、`touch/drivers/` |

优先用远程包。玻璃 init **数据**写在板级 `panellan_bsp_config.c`。仅在 IDF 内置 panel 之后补发 init 表的 helper（如 `lcd_drv/panel_st7789_vendor.c`）**不算**私有屏驱。

另见 `lcd/panel/README.md`、`touch/drivers/README.md`。

### 添加板型

1. 复制总线 / 屏 IC / 触摸最接近的现有板：

| 新板特征 | 优先复制 |
|----------|----------|
| RGB、无触摸 | `ZX4D30NE01S-UR-4827` |
| RGB + FT5x06 | `ZX3D95CE01S-TR-4848` |
| I80 + ST7796 + FT | `ZX3D50CE02S-USRC-4832` |
| I80 + NV3041 + CST | `ZX4D30CE08S-4827` |
| 编码器 | `ZX2D10GE01R-V-4848` |

```bash
cd target
cp -a <模板目录> <新板目录>
```

2. 填写 `target/<新板目录>/`：
   - `sdkconfig.defaults` — `CONFIG_ZX…=y`、分辨率、`CONFIG_PANELLAN_LCD_ENABLE_*` / `CONFIG_PANELLAN_LCD_PANEL_*` / `CONFIG_PANELLAN_TOUCH_*`
   - `panellan_bsp_pin.h` — GPIO 宏
   - `panellan_bsp_config.c` — 导出 `g_panellan_bsp_config`
   - 可选：`panellan_bsp_prepare()`（弱符号，默认空）

3. 注册（名称与 defaults 一致）：
   - `Kconfig.projbuild` — `choice EXAMPLE_LCD_INTERFACE` 增加 `config ZX…`
   - `CMakeLists.txt` — `elseif(CONFIG_ZX…)` 加入源文件 / include
   - 可选：`lcd/Kconfig.panellan` — H/V 分辨率 `default if ZX…`
   - 更新**仓库根** README 板一览表

4. 验证：

```bash
rm -f sdkconfig && rm -rf build
idf.py -DBOARD=<新板目录全名> set-target esp32s3 build
```

确认有 `PanelLan board:` / `panellan_bsp: <目录>`，串口有 `LCD ready …`。

### 添加远程屏驱 / 触摸

1. 在 `idf_component.yml` 增加 Registry 依赖，`rules` 绑定 `CONFIG_PANELLAN_*`。
2. 在 `lcd/Kconfig.panellan` 或 `touch/Kconfig.panellan` 增加选项。
3. 在 L2（`lcd/lcd_drv/` 或 `touch/`）接线创建 / init。
4. 板级 `sdkconfig.defaults` 打开选项，并在 `panellan_bsp_config.c` 填引脚与时序。

### 添加私有屏驱 / 触摸

仅当 Registry / esp-bsp 无对应包时（例：`touch/drivers/` 下 CST3240）：

1. 在 `lcd/panel/` 或 `touch/drivers/` 实现。
2. 在 `CMakeLists.txt` 用 IC Kconfig 条件编入。
3. 在 L2 头文件暴露 IC 枚举与 init。
4. 板级通过 config + defaults 选型。
