set(app_srcs "main.c")

idf_component_register(
    SRCS ${app_srcs}
    INCLUDE_DIRS "."
    REQUIRES driver esp_lcd esp_timer esp_driver_uart esp_driver_gpio panellan_bsp esp_lvgl_adapter
)
