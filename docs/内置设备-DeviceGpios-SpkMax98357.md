# DeviceGpios - SpkMax98357

MAX98357 I2S 功放，单例（`_MULTI_INSTANCE = false`）。

**源文件**：`src/device/gpio/Spk.Max98357.gpio.device.{h,cpp}`

**挂载点**：`EASYB.devicesGpio.spk`

## 引脚

| 宏 | 说明 |
|---|---|
| `PIN_GPIO_SPK_MAX98357_BCLK` | I2S 位时钟 |
| `PIN_GPIO_SPK_MAX98357_LRCK` | I2S 左右通道时钟 |
| `PIN_GPIO_SPK_MAX98357_DOUT` | I2S 数据输出 |

当前均为 `GPIO_NUM_NC`。

## API

| 方法 | 说明 |
|---|---|
| `setDeviceSampleRate(int rate)` | 采样率，boot 前设置，默认 24000 |
| `setDeviceVolume(int vol)` | 音量 0~100，曲线 `(vol/100)² × 65536` |
| `write(const vector<int16_t>& data)` | int16 × volFactor → int32 夹紧后 `i2s_write(portMAX_DELAY)` |

## 硬件资源

- **I2S_NUM_1**（TX），DMA 8 × 480
- boot 时 `i2s_driver_install` + `i2s_set_pin`，unboot 末持有者时 `i2s_driver_uninstall`
