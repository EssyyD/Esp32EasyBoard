# Esp32EasyBoard — 概述

## 介绍

这是一套**面向 ESP32 全家族的 PlatformIO 嵌入式固件框架**。项目源自 DIY 机器人玩具开发——每接一个新外设就重复写一遍引脚初始化、任务调度、总线管理，于是将这些模式抽成可复用的薄层。

框架的定位是为基于 ESP32 的智能设备（机器人、桌面摆件、IoT 终端等）提供"电池即用"的固件骨架——你只需关注具体外设的业务逻辑，引脚冲突检测、任务调度、设备引用计数、Web 管理台等通用能力已内置。

## 一句话概括

面向 ESP32 的模块化嵌入式框架：CRTP 设备抽象、FreeRTOS 服务调度、LVGL 显示屏、WiFi + Web API、SQLite 本地存储，以及丰富的设备驱动开箱可用。

## 核心能力

| 能力 | 说明 |
|---|---|
| **多总线设备抽象** | 统一的 CRTP 设备基类覆盖 GPIO / I2C / I2S / SPI，一行 `boot()` / `unboot()` 完成硬件生命周期管理 |
| **引用计数共享** | 多个服务可安全共享同一设备，框架自动追踪持有者，最后一处释放才真正关闭硬件 |
| **RTOS 服务框架** | 每个功能模块 = 一个 FreeRTOS task，提供 `_rtosBegin → _rtosLoop → _rtosEnd` 标准生命周期钩子 |
| **引脚冲突检测** | 运行时自动登记并检测 GPIO 重复占用，配合 `dumpPins()` 快速定位问题 |
| **总线单例** | I2C / SPI 总线由框架唯一持有，杜绝多实例冲突 |
| **丰富的设备库** | 持续扩充的驱动目录 (gpio / i2c / spi / i2s)，覆盖显示、音频、电机、传感器等常见外设 |
| **开箱即用的服务** | 预置 WiFi 联网、显示渲染、语音交互、运动控制等服务模块，可组合可裁剪 |
| **Web 管理台** | 内置 HTTP 静态站点 + JSON API 路由，可扩展自定义接口 |
| **本地数据库** | LittleFS 文件系统上的 SQLite3，支持建表、查询、行回调 |

## 运行环境

| 项目 | 要求 |
|---|---|
| **芯片** | ESP32 / ESP32-S2 / ESP32-S3 / ESP32-C3 / ESP32-C6（全家族） |
| **开发板** | 当前以 ESP32-S3 DevKitM-1 为主力验证，后续持续扩展板级配置 |
| **框架** | Arduino（ESP-IDF 底层能力直接可用） |
| **构建系统** | [PlatformIO](https://platformio.org/) |
| **C++ 标准** | GNU++17 |
| **Flash** | 16MB（QIO 模式，littlefs 分区约 6.9MB，可按需调整） |
| **PSRAM** | 8MB OPI（框架已适配，无需 PSRAM 的芯片可关闭对应选项） |
| **串口** | USB HW-CDC，115200bps |

## 外部依赖

| 库 | 用途 |
|---|---|
| [lvgl/lvgl](https://github.com/lvgl/lvgl) | GUI 框架 |
| [bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson) | JSON 序列化 |
| [siara-cc/Sqlite3Esp32](https://github.com/siara-cc/Sqlite3Esp32) | LittleFS 上的 SQLite |
| [adafruit/Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel) | WS2812 板载 LED |
| 其他 Adafruit 库 | SSD1306 OLED、GFX 图形基础（按需启用） |
| ESP-IDF 组件（内置） | I2S、LEDC、FreeRTOS、WiFi、SPI |

> 所有 PlatformIO 依赖在 `platformio.ini` 中声明，`pio run` 首次运行会自动下载。
