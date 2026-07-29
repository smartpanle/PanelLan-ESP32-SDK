# panel/ — private panel drivers

Place **panel drivers that are not on the Espressif Component Registry and are used only in this repo** (full `esp_lcd_panel_t` implementations) here.

| Put here | Do not put here |
|----------|-----------------|
| Private / local-fork `esp_lcd_new_panel_*` sources | Shared bus implementations (→ `lcd_drv/`) |
| | Official packages pulled via `idf_component.yml` (e.g. `esp_lcd_st7796`) |
| | Helpers that only resend init tables (e.g. existing `lcd_drv/panel_st7789_vendor.c`) |

Board glass init **data** still lives in `panellan_bsp_config.c` as `vendor_init_cmds`.

Integration steps: see [`../../README.md`](../../README.md) (Extending).
