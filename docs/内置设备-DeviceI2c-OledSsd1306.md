# DeviceI2c - OledSsd1306

0.91" 128×32 I2C OLED 显示屏，单例（`_MULTI_INSTANCE = false`）。

**源文件**：`src/device/i2c/Oled.Ssd1306.i2c.device.{h,cpp}`

**挂载点**：`EASYB.devicesI2c.oledSsd13065`

## 总线

走 `EASYB.i2c0`（硬件 I2C），地址 `0x3C`。依赖 Adafruit SSD1306 + Adafruit GFX。

## API

| 方法 | 说明 |
|---|---|
| `print(const char* text)` | 清屏后在顶部打印一行文本 |
| `clear()` | 清屏 |

## 参数

| 参数 | 值 |
|---|---|
| 分辨率 | 128 × 32 px |
| I2C 地址 | 0x3C |
