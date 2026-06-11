# 构建 / 烧录 / 引脚

## platformio.ini 要点

```ini
[env:esp32-s3-devkitm-1]
platform = espressif32          framework = arduino
board = esp32-s3-devkitm-1
board_build.flash_size = 16MB   flash_mode = qio   f_flash = 80MHz   f_cpu = 240MHz
board_build.partitions = default_16MB.csv          # app×2 + ~6.9MB LittleFS
board_build.psram_type = opi    arduino.memory_type = qio_opi   # 8MB OPI PSRAM
board_build.filesystem = littlefs
upload_port / monitor_port = /dev/ttyACM0          # Linux 命名;macOS 上是 /dev/cu.usbmodem*
monitor_speed = 115200          upload_speed = 921600
```

build_flags:

| flag | 作用 |
|---|---|
| `-std=gnu++17` | 项目语言标准 |
| `-DBOARD_HAS_PSRAM` + `-mfix-esp32-psram-cache-issue` | 启用 PSRAM(大 malloc 走 PSRAM) |
| `-DARDUINO_USB_CDC_ON_BOOT=0` + `-DARDUINO_USB_MODE=1` | Serial 走 USB HW-CDC/JTAG,不在 boot 抢 CDC |
| `-Iinclude -DLV_CONF_INCLUDE_SIMPLE=1` | 让 lvgl 找到 include/lv_conf.h |

lib_deps:Adafruit NeoPixel ^1.12 / SSD1306 ^2.5.7 / GFX ^1.11.9、ArduinoJson ^7、
Sqlite3Esp32 ^2.5、lvgl ^9.3。(Arduino_GFX 已移除,NV3007 用原生 SPI。)

## 常用命令(在 esp32S3Dev/ 目录下)

```bash
pio run                    # 编译
pio run -t upload          # 编译 + 烧录固件
pio run -t uploadfs        # 打包 data/ → LittleFS 烧录(改了 GIF/前端必须跑)
pio device monitor         # 串口监视(115200)
pio run -t clean           # 清理
```

macOS 上若 /dev/ttyACM0 不存在,用 `pio device list` 找端口,
临时覆盖:`pio run -t upload --upload-port /dev/cu.usbmodemXXXX`。

## 当前引脚分配(config/pinGpios.config.h)

### 已接线

| GPIO | 用途 |
|---|---|
| 41 / 42 | I2C0 SDA / SCL(400kHz) |
| 1 / 2 | HSPI MOSI / SCK(MISO 未用) |
| 4 / 5 / 6 | LCD NV3007 #1 DC / CS / RST |
| 48 | 板载 NeoPixel WS2812 |
| 19 / 20 / 21 | TB6612 Rotate L1:PWMA / AIN1 / AIN2 |
| 7 | TB6612 Rotate STBY |
| 13 / 14 | N20 Rotate L1 编码器 A / B |

### 占位(GPIO_NUM_NC,待接线)

FSPI 全部、LCD #2(CS/DC/RST)、INMP441 麦克风、MAX98357 喇叭、SSD1306 OLED(走 i2c0,仅地址 0x3C 有效)、
TB6612 Wheels 全部、N20 Wheel1/Wheel2/RotateL2 编码器。

### 已知的坑

- `PIN_GPIO_DRIVER_TB6612_ROTATE_L2_IN2` 写的是 `GPIO_NUM_N45`(**笔误**,非法宏)——因当前未被引用所以能编译;启用 Rotate L2 前必须改为 `GPIO_NUM_45`。
- `PIN_GPIO_DRIVER_TB6612_ROTATE_L2_IN1 = GPIO_NUM_48` 与 NeoPixel 冲突、`ROTATE_L2_PWM = GPIO_NUM_47`——这些是占位值,接线前要重新规划;运行期 `bootGPIO` 会打 `[GPIO] CONFLICT` 日志但**不会阻止运行**。
- 注释里 "Motor N20 Wheel1 Encoder C1=GPIO15 C2=GPIO16" 是预留接线建议,宏当前仍为 NC。
- I2S 控制器:I2S_NUM_0 = Mic、I2S_NUM_1 = Spk,S3 没有第三个;LEDC 通道 0/1 = TB6612。
- EasyBoard 的 GPIO 注册表上限 `MAX_PINS = 32`。

### 改引脚的正确姿势

1. 只改 `config/pinGpios.config.h` 的宏(设备代码不写裸引脚号)
2. 查上表确认无冲突;ESP32-S3 注意:GPIO 26~32 接内部 flash 不可用,33~37 在 OPI PSRAM 下被占,0/3/45/46 是 strapping 脚谨慎使用
3. 烧录后看 `EASYB.dumpPins()` 输出的 `=== GPIO Allocations ===` 与 `[GPIO] CONFLICT` 日志验证

## 串口日志约定

各模块日志前缀:`[EasyBoard] [BOOT] [UNBOOT] [GPIO] [RTOS] [Motion] [NV3007] [Joint] [flushJ] [GifPlayer] [LVGL-n] [Wifi] [ChipWebServerRtos] [ChipSqlite] [I2cSoft] [main]`。
排查启动问题先看 `=== GPIO Allocations ===` 之后的 boot 链日志。
