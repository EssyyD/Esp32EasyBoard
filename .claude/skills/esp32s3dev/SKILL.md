---
name: esp32s3dev
description: Use when working on the esp32S3Dev firmware project (ESP32-S3 DevKitM-1, PlatformIO + Arduino) — adding or modifying devices/drivers (SPI/I2C/I2S/GPIO), RTOS services, LVGL display, NV3007 LCD, GIF animations, WiFi/web API, SQLite, pin configuration, or building/flashing. Triggers include EasyBoard, EASYB, DeviceBase, RtosBase, ChipWebServer, ChipSqlite, GifFileLvglPlayer, boot/unboot, pinGpios.config.
---

# esp32S3Dev 项目技能

## 项目概述

`esp32S3Dev/` 是一个 ESP32-S3 机器人玩具固件(PlatformIO + Arduino 框架,gnu++17):
双 NV3007 LCD 拼接播放 GIF 表情、INMP441 麦克风 + MAX98357 喇叭语音回声、
TB6612 + N20 编码器电机运动、WiFi AP + Web 管理台(Vue3)+ SQLite 存储。

**核心架构(自上而下)**:

```
main.cpp            仅做 EASYB.init() + 选择性 service.rtosRun()
service.rtos/       业务服务,每个 = 一个 FreeRTOS task(继承 RtosBase)
device/             硬件驱动,按总线分目录(gpio/ i2c/ spi/),boot/unboot 引用计数
core/EasyBoard      全局单例 EASYB:唯一持有 SPI/I2C 总线、设备实例、GPIO 冲突注册表
core/base/          RtosBase / DeviceBase 家族(CRTP 模板)/ I2cSoftBase
config/             pinGpios.config.h(引脚表)+ deviceSets.*.config.h(服务参数)
data/               LittleFS 镜像:chipWeb 前端 + gifAnims GIF 资源
```

## 铁律(违反会导致运行时失效或编译错误)

1. **总线单例**:`Wire.begin()` 和 `new SPIClass` 只能出现在 `EasyBoard::initI2c/initSpi`。设备在自己的 `initI2c()/initSpi()` 里取 `EASYB.i2c0` / `EASYB.spiH`,绝不自建总线。
2. **模板实例化**:设备 `.cpp` 第一行必须 `#include "core/base/DeviceBase.<bus>.cpp"`(基类模板的定义在 .cpp 中,靠包含完成实例化)。
3. **引脚只在 `config/pinGpios.config.h` 定义**,未接线写 `GPIO_NUM_NC`(框架自动跳过 NC)。
4. **设备 boot/unboot 必须配对**:服务在 `_rtosBegin()` 里 `device->boot(serviceName)`,在 `_rtosEnd()` 里 `unboot(serviceName)`。首个 boot 才做硬件初始化,末个 unboot 才释放——引用计数按 serviceName 去重。
5. **`_rtosBegin()` 跑在主线程、task 创建之前,禁止 vTaskDelay**;需要 delay 的一次性逻辑放 `_rtosBeforeLoopOnce()`。
6. **新设备/服务必须挂到 `EasyBoard.h`** 的 `devicesGpio/devicesI2c/devicesSpi/servicesRtos` 结构体里,经 `instance()` 工厂创建(`_MULTI_INSTANCE=false` 用 static 单例,true 用 new)。

## 参考文档索引

| 文件 | 内容 | 何时读 |
|---|---|---|
| [references/architecture.md](references/architecture.md) | 目录结构、命名约定、CRTP/instance() 设计、启动流程 | 任何改动前先读 |
| [references/core-framework.md](references/core-framework.md) | RtosBase 生命周期、DeviceBase 引用计数、EasyBoard API、I2cSoftBase | 改框架/排查 boot 问题 |
| [references/devices.md](references/devices.md) | 全部设备清单 + API + **新增设备完整模板** | 新增/修改驱动 |
| [references/services.md](references/services.md) | 全部服务清单 + 参数 + **新增服务完整模板** | 新增/修改服务 |
| [references/display-lvgl.md](references/display-lvgl.md) | NV3007 驱动、双屏拼接 flush、GifFilePlayer、lv_conf 要点 | 显示/GIF/LVGL 相关 |
| [references/connectivity-web-data.md](references/connectivity-web-data.md) | WiFi 设备、Web API 协议、ChipSqlite、chipWeb 前端联动 | 联网/接口/存储 |
| [references/build-flash.md](references/build-flash.md) | platformio.ini 详解、构建烧录命令、引脚分配现状与坑 | 构建/烧录/换引脚 |

## 常见任务速查

- **接一个新传感器/外设** → devices.md「新增设备」+ build-flash.md 引脚表
- **加一个后台功能** → services.md「新增服务」,再到 main.cpp 调 `rtosRun()`
- **换 GIF 表情 / 改屏幕布局** → display-lvgl.md;GIF 放 `data/gifAnims/`,跑 `pio run -t uploadfs`
- **加一个 Web API** → connectivity-web-data.md「apiRouter 扩展」
- **编译烧录** → `pio run -t upload`;改了 `data/` 还要 `pio run -t uploadfs`

---

## 命名规范

| 类别 | 规范 | 示例 |
|---|---|---|
| 文件名 | `Type.Model.componenttype.ext` | `Mic.Inmp441.gpio.device.h` |
| 类名 | `TypeModelComponentType` | `MicInmp441GpioDevice` |
| 引脚宏 | `PIN_GPIO_DEVICE_PINNAME` | `PIN_GPIO_I2C_SDA` |
| 参数宏 | `DEVICE_SET_SERVICE_DEVICE_ITEM` | `DEVICE_SET_VOICE_ECHO_MIC_SAMPLE_RATE` |
| 变量 | camelCase | `sampleRate` |
| 常量/宏 | UPPER_SNAKE | `GPIO_NUM_NC` |

---

## 代码风格

- **main.cpp** 只有 `setup()` + `loop()`
- 每个类一个 public / private / protected 块，属性先于方法
- 成对方法连续书写（如 `boot()` / `unboot()`）
- 构造函数不写业务逻辑
- 每个 `.h` 必须有对应的 `.cpp`，.h 只声明，.cpp 定义

### 函数调用格式

单行：`func(a, b, c)`

多行：每参数独立一行同级缩进，左括号不换行，右括号与 func 对齐：
```cpp
func(
    aaaa,
    bbbbbbbbbbbbbbb,
    cccc
)
```

### 别名变量规则

- 推荐直接书写 `EASYB.xxx`
- 仅在同一 `{}` 范围内重复使用 10 次以上才定义别名（如 `auto& mic = EASYB.devicesGpio.mic;`）

### 模板 .cpp 包含规则

设备基类是模板，`.cpp` 文件第一行必须：
```cpp
#include "core/base/DeviceBase.spi.cpp"   // ← 按所属总线
#include "Xxx.spi.device.h"
```
漏掉会在链接期报 undefined reference。
