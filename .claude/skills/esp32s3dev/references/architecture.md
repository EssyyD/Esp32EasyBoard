# 架构与约定

## 仓库布局(esp32/ 根)

```
esp32/
├── esp32S3Dev/          # 本技能描述的固件工程(PlatformIO)
├── chipWeb/             # Web 管理台前端(Vue3 + Vite),build 产物拷入固件 data/chipWeb
├── tools/gif2cpp/       # GIF → RGB565 转换脚本(Python + Pillow)
├── pcbs/                # PCB 资料(当前为空)
└── working_resources/   # 工作素材(当前为空)
```

## 固件源码树(esp32S3Dev/)

```
platformio.ini               # env: esp32-s3-devkitm-1(见 build-flash.md)
include/lv_conf.h            # LVGL v9 配置(-DLV_CONF_INCLUDE_SIMPLE)
data/                        # LittleFS 镜像(pio run -t uploadfs)
│   ├── chipWeb/             #   前端静态资源(index.html + assets/)
│   └── gifAnims/blackE/     #   20 个表情 GIF
src/
├── main.cpp                 # setup(): EASYB.init() + 按需 service.rtosRun(); loop() 永久阻塞
├── config/
│   ├── config.h             # 聚合头,仅 include 下面两个
│   ├── pinGpios.config.h    # 全部 GPIO 引脚宏(唯一定义处,未接线 = GPIO_NUM_NC)
│   └── deviceSets.voiceEcho.config.h  # VoiceEcho 服务参数宏
├── core/
│   ├── EasyBoard.{h,cpp}    # 全局板级单例(宏 EASYB),总线/设备/服务/引脚注册表
│   ├── ChipWebServer.rtos.{h,cpp}  # HTTP 服务(静态站点 + JSON API),独立 RTOS task
│   ├── ChipSqlite.{h,cpp}   # SQLite 封装(LittleFS 上的 /chipSqlite/chipSqlite.db)
│   └── base/
│       ├── RtosBase.{h,cpp}         # FreeRTOS task mixin(rtosRun/rtosStop + 钩子)
│       ├── RtosBase.{h,cpp} # 服务基类 = RtosBase + serviceName + serviceDelay
│       ├── DeviceBase.{h,cpp}       # 设备基类:boot/unboot 引用计数
│       ├── DeviceBase.gpio.{h,cpp}  # CRTP:boot 时向 EASYB 注册 PIN_GPIOS
│       ├── DeviceBase.i2c.{h,cpp}   # CRTP:boot 时调子类 initI2c()
│       ├── DeviceBase.spi.{h,cpp}   # CRTP:boot 时注册引脚+置 OUTPUT/HIGH+调 initSpi()
│       └── I2cSoftBase.h            # GPIO 模拟 I2C 时序(软 I2C 总线基类)
├── device/
│   ├── Wifi.esp32S3.device.{h,cpp}  # WiFi AP/STA(芯片内置,不挂总线目录)
│   ├── gpio/   # Mic.Inmp441 / Spk.Max98357 / DriverMotor.Tb6612 / Motor.N20
│   ├── i2c/    # Oled.Ssd1306
│   └── spi/    # Lcd.Nv3007
├── module/                      # 可复用软件组件(不属 service/device/core)
│   └── GifFileLvglPlayer.h      # GIF 播放器(AnimatedGIF + LVGL)
├── i2cSofts/
│   └── I2cSoftExample.h     # 软 I2C 总线实例示例(SDA=17 SCL=18)
└── service.rtos/
    ├── Display.service.rtos.{h,cpp}       # LVGL + 双屏拼接 + GIF 播放
    ├── Motion.service.rtos.{h,cpp}        # 电机测试(前进/后退循环 + 编码器打印)
    ├── VoiceEcho.service.rtos.{h,cpp}     # VAD 录音→回放
    └── Connectivity.service.rtos.{h,cpp}  # WiFi AP 开启
```

## 命名约定

| 对象 | 文件名 | 类名 | 示例 |
|---|---|---|---|
| 设备 | `<Thing>.<Chip>.<bus>.device.{h,cpp}` | `<Thing><Chip><Bus>Device` | `Lcd.Nv3007.spi.device.h` → `LcdNv3007Device` |
| 服务 | `<Name>.service.rtos.{h,cpp}` | `<Name>ServiceRtos` | `Motion.service.rtos.h` → `MotionServiceRtos` |
| 引脚宏 | `PIN_GPIO_<DEVICE>_<信号>` | — | `PIN_GPIO_LCD_NV3007_CS` |
| 服务参数宏 | `DEVICE_SET_<SERVICE>_<参数>` | — | `DEVICE_SET_VOICE_ECHO_SPK_VOLUME` |
| 设备实例名(deviceName) | `"<Thing>.<CHIP>[.后缀]"` | — | `"Lcd.NV3007_2"`、`"Driver.TB6612.Wheels"` |
| 钩子/私有方法 | 前缀 `_` | — | `_rtosLoop`、`_spiCmd` |

代码风格:成员访问显式 `this->`;Serial 日志带 `[Tag]` 前缀。

## 关键设计

### 1. EASYB 单例中枢

`EasyBoard::BOARD` 静态实例,宏 `EASYB` 全局可用。它**独占**:

- `spiH`(HSPI)/ `spiF`(FSPI)—— 项目中唯一 `new SPIClass` 处
- `i2c0`(= &Wire)—— 项目中唯一 `Wire.begin()` 处
- 所有设备实例(按总线分组的匿名 struct:`devicesI2c/devicesSpi/devicesGpio/i2cSofts`)
- 所有服务实例(`servicesRtos`)+ `chipWebServerRtos` + `chipSqlite` + `wifi` + `neoPixel`
- GPIO 冲突注册表 `pinsAlloc[32]`(`bootGPIO/unbootGPIO/dumpPins`)

### 2. CRTP 设备基类 + 模板 .cpp 包含

`DeviceGpioBase<Derived>` / `DeviceI2cBase<Derived,BusType>` / `DeviceSpiBase<Derived,BusType>`
是模板,**成员函数定义放在 `.cpp` 文件里**。因此每个设备实现文件第一行必须:

```cpp
#include "core/base/DeviceBase.spi.cpp"   // ← 按所属总线,必须在最前
#include "Lcd.Nv3007.spi.device.h"
```

漏掉会在链接期报 undefined reference。

### 3. instance() 工厂 + _MULTI_INSTANCE

每个设备声明 `static constexpr bool _MULTI_INSTANCE`:

- `false`(单例):`instance()` 返回函数内 `static` 实例(如 Mic、Spk、TB6612、SSD1306)
- `true`(多实例):`instance()` 每次 `new`(如 NV3007 双屏、N20 多电机)

`instance()` 只赋值 deviceName/引脚等参数,**不做硬件初始化**——初始化在 `boot()`。

### 4. PIN_GPIOS 引脚声明

设备引脚写成嵌套 struct,可被基类 range-for 遍历:

```cpp
struct Pins {
    PinDef cs {"Lcd.NV3007.CS",  GPIO_NUM_NC};   // owner 字符串用于冲突日志
    PinDef dc {"Lcd.NV3007.DC",  GPIO_NUM_NC};
    PinDef* begin() { return &cs; }
    PinDef* end()   { return &dc + 1; }           // 末成员 + 1
} PIN_GPIOS;
```

`bootGPIO` 对 `GPIO_NUM_NC` 直接放行,冲突时打印 `[GPIO] CONFLICT` 并返回 false(仅告警,不中止)。

## 启动流程

```
setup()
 ├─ EASYB.init()
 │   ├─ Serial.begin(115200) + LittleFS.begin(false)
 │   ├─ NeoPixel(GPIO48) 初始化,亮度 32
 │   ├─ initI2c(): Wire.begin(SDA=41, SCL=42) @400kHz(引脚非 NC 才执行)
 │   ├─ initSpi(): spiH = new SPIClass(HSPI), begin(SCK=2, MISO=NC, MOSI=1);spiF 同理(当前 NC 不启)
 │   └─ onboardDeviceGpio(): 给 TB6612 挂 N20 电机实例 + 给 mic/spk/tb6612 注入 EasyBoard 指针
 ├─ EASYB.dumpPins()                  # 打印 GPIO 分配表
 ├─ EASYB.servicesRtos.<某服务>.rtosRun()   # 在 main.cpp 注释/取消注释来选择启用的服务
 └─ loop(): vTaskDelay(portMAX_DELAY)  # Arduino loop 永不工作
```

服务启停完全由 main.cpp(或运行期代码)调 `rtosRun()/rtosStop()` 控制,没有自动编排。
