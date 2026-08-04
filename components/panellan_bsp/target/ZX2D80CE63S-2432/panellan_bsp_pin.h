#pragma once

// BOARD BASE PIN
#define BOARD_RESET_PIN     (25)
#define BOARD_I2C_SDA_PIN   (27)
#define BOARD_I2C_SCL_PIN   (28)

// LCD PIN
#define LCD_BL_PIN          (22)

// LCD SPI PIN
#define LCD_RST_PIN         (25)
#define LCD_CS_PIN          (9)
#define LCD_SCL_PIN         (3)
#define LCD_SDA_PIN         (24)
#define LCD_RS_PIN          (2)
#define LCD_TE_PIN          (23)

// TOUCH PAD PIN
#define TP_I2C_SDA_PIN      (BOARD_I2C_SDA_PIN)
#define TP_I2C_SCL_PIN      (BOARD_I2C_SCL_PIN)
#define TP_I2C_INT_PIN      (26)
#define TP_I2C_RST_PIN      (-1)

// USB
#define USB_DP_PIN          (13)
#define USB_DN_PIN          (12)

// UART-485
#define RS485_RTS_PIN       (5)
#define RS485_RXD_PIN       (4)
#define RS485_TXD_PIN       (8)

// EXTERNAL PIN
#define EXTERNAL_PIN_1     (29)
#define EXTERNAL_PIN_2     (1)
#define EXTERNAL_PIN_3     (0)
#define EXTERNAL_PIN_4     (7)
#define EXTERNAL_PIN_5     (6)
