# DeviceSpi - LcdNv3007

NV3007 1.78" 条形 TFT LCD（142×428），4 线 SPI 接口，多实例（`_MULTI_INSTANCE = true`，支持双屏拼接）。

**源文件**：`src/device/spi/Lcd.Nv3007.spi.device.{h,cpp}`

**挂载点**：`EASYB.devicesSpi.lcdNv3007`（主屏）、`EASYB.devicesSpi.lcdNv3007_2`（副屏，当前引脚 NC）。

## 引脚

| 宏 | 说明 |
|---|---|
| `PIN_GPIO_LCD_NV3007_CS` | 片选 |
| `PIN_GPIO_LCD_NV3007_DC` | 数据/命令选择 |
| `PIN_GPIO_LCD_NV3007_RST` | 硬件复位 |

SPI 时钟和数据走 `EASYB.spiH`（SCK=2, MOSI=1）。

## SPI 参数

- 总线：`EASYB.spiH` @ 50MHz，`SPI_MODE3`，MSBFIRST
- 原生 `SPIClass` 控制 CS/DC，不依赖 Arduino_GFX

## API

| 方法 | 说明 |
|---|---|
| `initLcdV()` | 竖屏 142×428，`coordMode='V'` |
| `initLcdH()` | 横屏 428×142，`coordMode='H'` |
| `flushWrite(x1,y1,x2,y2, pxMap, len)` | 唯一像素出口：CASET → RASET → 写 RGB565 |

## 坐标偏移

`flushWrite` 自动补偿驱动 IC 的 CASET/RASET 偏移：

| 模式 | CASET 偏移 | RASET 偏移 |
|---|---|---|
| 竖屏 | 12 | 0 |
| 横屏 | `hCasetOffset` | 12 |

`hCasetOffset` 由 `instance()` 传入：主屏 = 0，副屏 = 14。

## 初始化

`_tftInit()` 执行完整厂商序列（私有寄存器页 → gamma/电源/扫描 → Sleep Out → Display On），约 420ms。

## 双屏

副屏通过 `LcdNv3007Device::instance("Lcd.NV3007_2", ..., ROTATION1_OFFSET)` 创建，横屏 offset=14。DisplayServiceRtos 通过 `_lvglToJoint()` 将两块屏拼成 LVGL 逻辑大屏。
