# touch/ — private touch drivers

Place **touch IC drivers that are not on the Espressif Component Registry and are used only in this repo** here.

| Put here | Do not put here |
|----------|-----------------|
| Private / local-fork `esp_lcd_touch_*` implementations | Official packages pulled via `idf_component.yml` (e.g. `esp_lcd_touch_ft5x06`) |
| Board-specific touch calibration or patch sources | Shared orientation logic (→ `helpers/`) |

Integration steps: see [`../../README.md`](../../README.md) (Extending).

## Existing private drivers

| IC | Path | Notes |
|----|------|-------|
| CST3240 | `esp_lcd_touch_cst3240.c` + `include/` | **Not on Registry / esp-bsp** (bsp only has CST816S). In-repo `esp_lcd_touch` wrapper. |
