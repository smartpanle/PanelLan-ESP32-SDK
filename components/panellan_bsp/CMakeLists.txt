# Unified PanelLan BSP: board (L3) + lcd (L2) + touch (L2)
# Note: idf_component_register ignores SRC_DIRS when SRCS is set — list all files in SRCS.

set(srcs
    "board/panellan_bsp.c"
    "board/panellan_bsp_hw.c"
    "lcd/panellan_lcd.c"
    "lcd/helpers/panellan_lcd_orientation.c"
    "lcd/helpers/panellan_lcd_panel.c"
    "touch/panellan_touch.c"
    "touch/helpers/panellan_touch_orientation.c"
)
set(include_dirs "include")
set(exclude_srcs)
set(requires driver esp_lcd esp_timer esp_lcd_touch)

# ---- LCD bus ----
if(CONFIG_PANELLAN_LCD_ENABLE_RGB)
    list(APPEND srcs "lcd/lcd_drv/panellan_lcd_rgb.c" "lcd/lcd_drv/panellan_lcd_spi9.c")
endif()

if(CONFIG_PANELLAN_LCD_ENABLE_I80)
    list(APPEND srcs
        "lcd/lcd_drv/panellan_lcd_i80.c"
        "lcd/lcd_drv/panellan_lcd_i80_flush.c"
        # "lcd/lcd_drv/panellan_lcd_i80_gdma_wrap.c"
    )
endif()

if(CONFIG_PANELLAN_LCD_ENABLE_SPI_WITH_PSRAM OR CONFIG_PANELLAN_LCD_ENABLE_SPI_WITHOUT_PSRAM)
    list(APPEND srcs "lcd/lcd_drv/panellan_lcd_spi.c")
endif()

if(CONFIG_PANELLAN_LCD_ENABLE_QSPI)
    list(APPEND srcs "lcd/lcd_drv/panellan_lcd_qspi.c")
endif()

if(CONFIG_PANELLAN_LCD_ENABLE_MIPI_DSI)
    list(APPEND srcs "lcd/lcd_drv/panellan_lcd_mipi_dsi.c")
    list(APPEND requires esp_ldo_regulator)
endif()



# ---- LCD panel drivers ----
if(CONFIG_PANELLAN_LCD_PANEL_ST7789)
    list(APPEND srcs "lcd/lcd_drv/panel_st7789_vendor.c")
endif()

if(CONFIG_PANELLAN_LCD_PANEL_ST7796)
    list(APPEND requires esp_lcd_st7796)
endif()
if(CONFIG_PANELLAN_LCD_PANEL_NV3041)
    list(APPEND requires esp_lcd_nv3041)
endif()
if(CONFIG_PANELLAN_LCD_PANEL_ILI9341)
    list(APPEND requires esp_lcd_ili9341)
endif()
if(CONFIG_PANELLAN_LCD_PANEL_GC9A01)
    list(APPEND requires esp_lcd_gc9a01)
endif()
if(CONFIG_PANELLAN_LCD_PANEL_ST77916)
    list(APPEND requires esp_lcd_st77916)
endif()
if(CONFIG_PANELLAN_LCD_PANEL_ST7701)
    list(APPEND requires esp_lcd_st7701)
endif()


# ---- Touch drivers ----
if(CONFIG_PANELLAN_TOUCH_FT5X06)
    list(APPEND requires esp_lcd_touch_ft5x06)
endif()
if(CONFIG_PANELLAN_TOUCH_GT1151)
    list(APPEND requires esp_lcd_touch_gt1151)
endif()
if(CONFIG_PANELLAN_TOUCH_GT911)
    list(APPEND requires esp_lcd_touch_gt911)
endif()
if(CONFIG_PANELLAN_TOUCH_CST3240)
    list(APPEND srcs "touch/drivers/esp_lcd_touch_cst3240.c")
    list(APPEND include_dirs "touch/drivers/include")
endif()

list(APPEND requires esp_lvgl_adapter)

# ---- Active board product sources (board config) ----
if(CONFIG_ZX2D10GE01R)
    set(_board_dir "target/ZX2D10GE01R-V-4848")
    list(APPEND srcs
        "${_board_dir}/panellan_bsp_config.c"
        "${_board_dir}/knob_init.c"
    )
    if(CONFIG_PANELLAN_BOARD_USE_ENCODER)
        list(APPEND srcs "${_board_dir}/lv_demo_encode.c")
    endif()
    list(APPEND include_dirs "${_board_dir}")
    message(STATUS "panellan_bsp: ZX2D10GE01R-V-4848")
elseif(CONFIG_ZX2D80CE02S)
    list(APPEND srcs "target/ZX2D80CE02S-2432/panellan_bsp_config.c")
    list(APPEND include_dirs "target/ZX2D80CE02S-2432")
    message(STATUS "panellan_bsp: ZX2D80CE02S-2432")
elseif(CONFIG_ZX2D80CECOF)
    list(APPEND srcs "target/ZX2D80CECOF-2432/panellan_bsp_config.c")
    list(APPEND include_dirs "target/ZX2D80CECOF-2432")
    message(STATUS "panellan_bsp: ZX2D80CECOF-2432")
elseif(CONFIG_ZX2D80CE63S)
    list(APPEND srcs "target/ZX2D80CE63S-2432/panellan_bsp_config.c")
    list(APPEND include_dirs "target/ZX2D80CE63S-2432")
    message(STATUS "panellan_bsp: ZX2D80CE63S-2432")
elseif(CONFIG_ZX3D50CE02S_USRC)
    list(APPEND srcs "target/ZX3D50CE02S-USRC-4832/panellan_bsp_config.c")
    list(APPEND include_dirs "target/ZX3D50CE02S-USRC-4832")
    message(STATUS "panellan_bsp: ZX3D50CE02S-USRC-4832")
elseif(CONFIG_ZX3D95CE01S_TR)
    list(APPEND srcs "target/ZX3D95CE01S-TR-4848/panellan_bsp_config.c")
    list(APPEND include_dirs "target/ZX3D95CE01S-TR-4848")
    message(STATUS "panellan_bsp: ZX3D95CE01S-TR-4848")
elseif(CONFIG_ZX4D30NE01S_UR)
    list(APPEND srcs "target/ZX4D30NE01S-UR-4827/panellan_bsp_config.c")
    list(APPEND include_dirs "target/ZX4D30NE01S-UR-4827")
    message(STATUS "panellan_bsp: ZX4D30NE01S-UR-4827")
elseif(CONFIG_ZX4D30CE08S)
    list(APPEND srcs "target/ZX4D30CE08S-4827/panellan_bsp_config.c")
    list(APPEND include_dirs "target/ZX4D30CE08S-4827")
    message(STATUS "panellan_bsp: ZX4D30CE08S-4827")
elseif(CONFIG_ZX7D00CE01S)
    list(APPEND srcs "target/ZX7D00CE01S-8048/panellan_bsp_config.c")
    list(APPEND include_dirs "target/ZX7D00CE01S-8048")
    message(STATUS "panellan_bsp: ZX7D00CE01S-8048")
endif()



set(_priv_requires)
if(CONFIG_PANELLAN_LCD_ENABLE_I80)
    list(APPEND _priv_requires esp_hw_support)
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${include_dirs}
    PRIV_INCLUDE_DIRS "lcd"
    REQUIRES ${requires}
    PRIV_REQUIRES ${_priv_requires}
)

idf_component_optional_requires(PRIVATE lvgl__lvgl lvgl)
