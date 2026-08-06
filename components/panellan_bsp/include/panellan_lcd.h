/*
 * PanelLan LCD L2 — bus/panel helpers over esp_lcd_*.
 * Board fills panellan_lcd_config_t (panel + typed bus union).
 * Bus layout mirrors tmp_sdk/components/hw_init/lcd/:
 *   RGB / I80 / SPI(+PSRAM) / SPI(-PSRAM) / QSPI / MIPI-DSI
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "hal/lcd_types.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Display rotation (independent of esp_lvgl_adapter).
 * Numeric values match esp_lv_adapter_rotation_t when LVGL is enabled.
 */
typedef enum {
    PANELLAN_DISPLAY_ROTATE_0 = 0,
    PANELLAN_DISPLAY_ROTATE_90,
    PANELLAN_DISPLAY_ROTATE_180,
    PANELLAN_DISPLAY_ROTATE_270,
} panellan_display_rotation_t;

/**
 * @brief LCD bus type
 */
typedef enum {
    PANELLAN_LCD_BUS_RGB = 0,               /*!< RGB parallel interface */
    PANELLAN_LCD_BUS_I80,                   /*!< Intel 8080 (I80) interface */
    PANELLAN_LCD_BUS_SPI_WITH_PSRAM,        /*!< SPI, draw buffers in PSRAM */
    PANELLAN_LCD_BUS_SPI_WITHOUT_PSRAM,     /*!< SPI, no PSRAM / partial buffers */
    PANELLAN_LCD_BUS_QSPI,                  /*!< Quad QSPI interface */
    PANELLAN_LCD_BUS_MIPI_DSI,              /*!< MIPI DSI (typically ESP32-P4) */
} panellan_lcd_bus_t;

/**
 * @brief Panel IC selection (set by board config; Kconfig gates which drivers build)
 */
typedef enum {
    PANELLAN_LCD_PANEL_RGB_ONLY = 0,        /*!< RGB passthrough, no separate panel IC */
    PANELLAN_LCD_PANEL_ST7789,              /*!< ST7789 */
    PANELLAN_LCD_PANEL_ST7796,              /*!< ST7796 */
    PANELLAN_LCD_PANEL_NV3041,              /*!< NV3041 */
    PANELLAN_LCD_PANEL_ILI9341,             /*!< ILI9341 */
    PANELLAN_LCD_PANEL_GC9A01,              /*!< GC9A01 */
    PANELLAN_LCD_PANEL_ST77916,             /*!< ST77916 (often used with QSPI) */
    PANELLAN_LCD_PANEL_ST7701,              /*!< ST7701 (often used with MIPI) */
} panellan_lcd_panel_ic_t;

/**
 * @brief RGB bounce buffer policy
 */
typedef enum {
    PANELLAN_LCD_BOUNCE_AUTO = 0,           /*!< Driver picks by resolution */
    PANELLAN_LCD_BOUNCE_NONE,               /*!< No bounce */
    PANELLAN_LCD_BOUNCE_CUSTOM,             /*!< Use bounce_buffer_size_px */
} panellan_lcd_bounce_mode_t;

/**
 * @brief SPI9 init command row (RGB panels often programmed via 9-bit SPI)
 */
typedef struct {
    uint8_t cmd;                        /*!< Command byte */
    const uint8_t *data;                /*!< Param buffer; NULL if none */
    uint8_t data_bytes;                 /*!< Param length (bytes) */
    uint16_t delay_ms;                  /*!< Delay after command (ms) */
} panellan_lcd_spi9_cmd_t;

/**
 * @brief Generic vendor init command row
 *
 * Layout matches common esp_lcd_*_lcd_init_cmd_t (ST7796 / NV3041 / ILI9341 / etc.),
 * so boards can fill tables without including panel-driver headers.
 */
typedef struct {
    int cmd;                            /*!< LCD command */
    const void *data;                   /*!< Command params; may be NULL */
    size_t data_bytes;                  /*!< Param length (bytes) */
    unsigned int delay_ms;              /*!< Delay after command (ms) */
} panellan_lcd_init_cmd_t;

/**
 * @brief Shared panel params (resolution, backlight, orientation, vendor table, etc.)
 */
typedef struct {
    int h_res;                          /*!< Horizontal resolution (pixels) */
    int v_res;                          /*!< Vertical resolution (pixels) */
    gpio_num_t bl_gpio;                 /*!< Backlight GPIO; GPIO_NUM_NC if unused */
    uint8_t bl_on_level;                /*!< Backlight active level (0/1) */
    bool turn_on_bl_after_init;         /*!< Auto-enable backlight after init */
    gpio_num_t te_gpio;                 /*!< TE pin; GPIO_NUM_NC if none */
    panellan_lcd_panel_ic_t panel_ic;       /*!< Panel IC */
    const void *vendor_init_cmds;       /*!< Vendor init table; usually panellan_lcd_init_cmd_t[] */
    size_t vendor_init_cmds_count;      /*!< Vendor table entry count */
    lcd_rgb_element_order_t color_order;/*!< Pixel color order (RGB/BGR) */
    uint8_t bits_per_pixel;             /*!< Color depth, e.g. 16 */
    bool apply_orientation;             /*!< Apply swap/mirror from rotation */
    bool base_swap_xy;                  /*!< Board default: swap XY */
    bool base_mirror_x;                 /*!< Board default: mirror X */
    bool base_mirror_y;                 /*!< Board default: mirror Y */
    bool invert_color;                  /*!< Invert colors */
    bool reset_active_high;             /*!< Reset active-high */
} panellan_lcd_panel_t;

/**
 * @brief RGB bus pins and timing
 */
typedef struct {
    gpio_num_t vsync;                   /*!< VSYNC */
    gpio_num_t hsync;                   /*!< HSYNC */
    gpio_num_t de;                      /*!< Data Enable */
    gpio_num_t pclk;                    /*!< Pixel clock */
    gpio_num_t disp;                    /*!< DISP enable; NC if unused */
    gpio_num_t data[16];                /*!< Data lines; unused pins NC */
    uint32_t pclk_hz;                   /*!< Pixel clock frequency (Hz) */
    uint16_t hsync_pulse_width;         /*!< HSYNC pulse width */
    uint16_t hsync_back_porch;          /*!< Horizontal back porch */
    uint16_t hsync_front_porch;         /*!< Horizontal front porch */
    uint16_t vsync_pulse_width;         /*!< VSYNC pulse width */
    uint16_t vsync_back_porch;          /*!< Vertical back porch */
    uint16_t vsync_front_porch;         /*!< Vertical front porch */
    bool pclk_active_neg;               /*!< PCLK active on falling edge */
    lcd_clock_source_t clk_src;         /*!< Clock source */
    panellan_lcd_bounce_mode_t bounce_mode; /*!< Bounce policy */
    size_t bounce_buffer_size_px;       /*!< Bounce size in pixels when CUSTOM */
    bool pulse_vsync_enable;            /*!< Enable pulse vsync */
    bool use_spi9_init;                 /*!< Send init cmds via SPI9 */
    gpio_num_t spi9_mosi;               /*!< SPI9 MOSI */
    gpio_num_t spi9_clk;                /*!< SPI9 CLK */
    gpio_num_t spi9_cs;                 /*!< SPI9 CS */
    spi_host_device_t spi9_host;        /*!< SPI9 host */
    const panellan_lcd_spi9_cmd_t *spi9_cmds; /*!< SPI9 command table */
    size_t spi9_cmd_count;              /*!< SPI9 command count */
} panellan_lcd_rgb_cfg_t;

/**
 * @brief I80/8080 bus parameters
 */
typedef struct {
    gpio_num_t dc;                      /*!< D/C (data/command) */
    gpio_num_t wr;                      /*!< WR write strobe */
    gpio_num_t cs;                      /*!< Chip select; may be NC */
    gpio_num_t rst;                     /*!< Reset; may be NC (or board manual reset) */
    gpio_num_t data[8];                 /*!< 8-bit data lines */
    uint32_t pclk_hz;                   /*!< Bus clock (Hz) */
    uint8_t bus_width;                  /*!< Bus width, typically 8 */
    size_t max_transfer_bytes;          /*!< Max bytes per transfer */
    uint8_t dma_burst_size;             /*!< DMA burst */
    lcd_clock_source_t clk_src;         /*!< Clock source */
    bool manual_reset;                  /*!< GPIO manual reset inside driver */
    bool reset_active_high;             /*!< Reset active-high */
    bool swap_color_bytes;              /*!< Hardware RGB565 byte swap */
    uint8_t wr_idle_level;              /*!< WR idle level */
    uint8_t trans_queue_depth;          /*!< Transfer queue depth */
    bool call_panel_reset;              /*!< Call esp_lcd_panel_reset */
} panellan_lcd_i80_cfg_t;

/**
 * @brief SPI LCD bus parameters (shared ±PSRAM)
 */
typedef struct {
    spi_host_device_t host;             /*!< SPI host */
    gpio_num_t sclk;                    /*!< SCLK */
    gpio_num_t mosi;                    /*!< MOSI */
    gpio_num_t miso;                    /*!< MISO; NC if unused */
    gpio_num_t cs;                      /*!< CS */
    gpio_num_t dc;                      /*!< D/C */
    gpio_num_t rst;                     /*!< RST */
    uint32_t pclk_hz;                   /*!< SPI clock (Hz) */
    size_t max_transfer_bytes;          /*!< Max bytes per transfer */
    uint8_t trans_queue_depth;          /*!< Transfer queue depth */
    uint8_t spi_mode;                   /*!< SPI mode 0~3 */
    uint8_t lcd_cmd_bits;               /*!< Command bit width */
    uint8_t lcd_param_bits;             /*!< Param bit width */
} panellan_lcd_spi_cfg_t;

/**
 * @brief QSPI LCD bus parameters
 */
typedef struct {
    spi_host_device_t host;             /*!< SPI/QSPI host */
    gpio_num_t sclk;                    /*!< SCLK */
    gpio_num_t data0;                   /*!< DATA0 / MOSI */
    gpio_num_t data1;                   /*!< DATA1 */
    gpio_num_t data2;                   /*!< DATA2 */
    gpio_num_t data3;                   /*!< DATA3 */
    gpio_num_t cs;                      /*!< CS */
    gpio_num_t dc;                      /*!< D/C (unused on some panels) */
    gpio_num_t rst;                     /*!< RST */
    gpio_num_t power_gpio;              /*!< Power control; NC if unused; see power_on_level */
    uint8_t power_on_level;             /*!< Power-on active level */
    uint32_t pclk_hz;                   /*!< Clock (Hz) */
    size_t max_transfer_bytes;          /*!< Max bytes per transfer */
    uint8_t trans_queue_depth;          /*!< Transfer queue depth */
    uint8_t lcd_cmd_bits;               /*!< Command bit width */
    uint8_t lcd_param_bits;             /*!< Param bit width */
} panellan_lcd_qspi_cfg_t;

/**
 * @brief MIPI DSI bus and DPI timing
 *
 * phy_clk_src differs across P4 chip revs. Use 0 so esp_lcd_new_dsi_bus picks
 * the revision-correct default (<3.0 → PLL_F20M, >=3.0 → XTAL). Do not use
 * MIPI_DSI_PHY_CLK_SRC_DEFAULT — it is LEGACY (PLL_F20M) and aborts on rev >= 3.0.
 */
typedef struct {
    int ldo_chan_id;                    /*!< PHY LDO channel */
    int ldo_voltage_mv;                 /*!< PHY LDO voltage (mV) */
    gpio_num_t rst;                     /*!< Panel reset */
    gpio_num_t pwr_gpio;                /*!< Power enable; NC if unused */
    uint8_t pwr_on_level;               /*!< Power-on level */
    uint8_t lane_num;                   /*!< MIPI lane count */
    uint32_t lane_bit_rate_mbps;        /*!< Lane bit rate (Mbps) */
    int phy_clk_src; /*!< PHY PLL ref clk; 0 = IDF auto by chip rev */
    uint32_t dpi_clock_freq_mhz;        /*!< DPI clock (MHz) */
    uint16_t hsync_back_porch;          /*!< Horizontal back porch */
    uint16_t hsync_pulse_width;         /*!< HSYNC pulse width */
    uint16_t hsync_front_porch;         /*!< Horizontal front porch */
    uint16_t vsync_back_porch;          /*!< Vertical back porch */
    uint16_t vsync_pulse_width;         /*!< VSYNC pulse width */
    uint16_t vsync_front_porch;         /*!< Vertical front porch */
} panellan_lcd_mipi_cfg_t;

/**
 * @brief Full LCD config: shared panel + bus-specific union member
 *
 * Bus config types are always present so board tables can compile even when
 * the corresponding driver Kconfig is off. Only driver sources are gated.
 */
typedef struct {
    panellan_lcd_bus_t bus;                 /*!< Bus type; selects which u member */
    panellan_lcd_panel_t panel;             /*!< Shared panel params */
    union {
        panellan_lcd_rgb_cfg_t rgb;         /*!< RGB bus config */
        panellan_lcd_i80_cfg_t i80;         /*!< I80 bus config */
        panellan_lcd_spi_cfg_t spi;         /*!< SPI bus config */
        panellan_lcd_qspi_cfg_t qspi;       /*!< QSPI bus config */
        panellan_lcd_mipi_cfg_t mipi;       /*!< MIPI DSI bus config */
    } u;                                /*!< Bus-specific config */
} panellan_lcd_config_t;

/* Create and init LCD (bus + panel) from cfg->bus; outputs panel/io handles.
 * tear_avoid_mode: when LVGL on, pass esp_lv_adapter_tear_avoid_mode_t;
 * when LVGL off, ignored for RGB/MIPI (fixed num_fbs). */
esp_err_t panellan_lcd_init(const panellan_lcd_config_t *cfg,
                        int tear_avoid_mode,
                        panellan_display_rotation_t rotation,
                        esp_lcd_panel_handle_t *out_panel,
                        esp_lcd_panel_io_handle_t *out_io);

/* Deinit current LCD; free bus and panel resources */
esp_err_t panellan_lcd_deinit(void);

/* Toggle backlight (uses panel.bl_gpio / bl_on_level) */
void panellan_lcd_set_bl(bool on);

/* Panel DISPON/DISPOFF; call after first LVGL frame to avoid white flash */
esp_err_t panellan_lcd_disp_on(bool on);

/* TE GPIO number, or GPIO_NUM_NC if unset */
int panellan_lcd_get_te_gpio(void);

/* Current bus clock (Hz); 0 if unknown */
uint32_t panellan_lcd_get_bus_freq_hz(void);

/* Bus data-line count (e.g. I80=8, RGB effective width) */
uint8_t panellan_lcd_get_bus_data_lines(void);

/* Color depth bits_per_pixel */
uint8_t panellan_lcd_get_bits_per_pixel(void);

#if CONFIG_PANELLAN_LCD_ENABLE_I80
/* Allocate I80 DMA-friendly draw buffer (internal/PSRAM per caps) */
void *panellan_lcd_i80_alloc_draw_buffer(size_t size, uint32_t caps);

/* Pass lv_display_t* from caller (include lvgl.h there). */
esp_err_t panellan_lcd_i80_setup_adapter_flush(void *disp, esp_lcd_panel_io_handle_t io);
esp_err_t panellan_lcd_i80_bind_lvgl_draw_buffer(void *disp);
#endif

/* Current esp_lcd panel handle; NULL if not initialized */
esp_lcd_panel_handle_t panellan_lcd_get_panel(void);

/* Current panel IO handle (may be NULL for RGB passthrough) */
esp_lcd_panel_io_handle_t panellan_lcd_get_io(void);

/* Horizontal resolution */
uint16_t panellan_lcd_get_h_res(void);

/* Vertical resolution */
uint16_t panellan_lcd_get_v_res(void);

/* Compose board base orientation with display rotation into final swap_xy / mirror_x / mirror_y */
void panellan_lcd_get_orientation_flags(bool base_swap_xy, bool base_mirror_x, bool base_mirror_y,
                                    panellan_display_rotation_t rotation,
                                    bool *swap_xy, bool *mirror_x, bool *mirror_y);

#ifdef __cplusplus
}
#endif
