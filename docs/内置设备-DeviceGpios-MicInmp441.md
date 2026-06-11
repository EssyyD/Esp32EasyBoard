# DeviceGpios - MicInmp441

INMP441 I2S MEMS 麦克风，单例（`_MULTI_INSTANCE = false`）。

**源文件**：`src/device/gpio/Mic.Inmp441.gpio.device.{h,cpp}`

**挂载点**：`EASYB.devicesGpio.mic`

## 引脚

| 宏 | 说明 |
|---|---|
| `PIN_GPIO_MIC_INMP441_SCK` | I2S 时钟 |
| `PIN_GPIO_MIC_INMP441_WS` | I2S 字选 |
| `PIN_GPIO_MIC_INMP441_DIN` | I2S 数据输入 |

当前均为 `GPIO_NUM_NC`，接线后需更新 `pinGpios.config.h`。

## API

| 方法 | 说明 |
|---|---|
| `setDeviceSampleRate(int rate)` | 设置采样率，**必须在 boot 前调用**，默认 16000 |
| `read(vector<int16_t>& data, size_t samples)` | 读 I2S 数据，32bit 右移 12 位后夹紧到 int16，超时 500ms |

## 硬件资源

- **I2S_NUM_0**（RX），DMA 8 × 480
- boot 时 `i2s_driver_install` + `i2s_set_pin`，unboot 末持有者时 `i2s_driver_uninstall`
