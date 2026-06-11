# 内置设备 DeviceI2c - 概述

I2C 设备继承 `DeviceI2cBase<Derived, BusType>`（CRTP 模板基类）。

## 获取总线

基类已提供默认实现，`this->i2c` 自动指向 `EASYB.i2c0`。**绝大多数情况下无需重写**。

> ⚠ **严禁在设备中自行 `Wire.begin()`。** 框架已在 `EasyBoard::initI2c()` 中完成 `Wire.begin()`，自行调用会破坏所有依赖 `EASYB.i2c0` 的设备。

只有使用非标准 I2C（如软件模拟 I2C、额外 I2C 控制器）时才需重写 `initI2c()`，将 `this->i2c` 指向你的自定义实例即可。

## i2cAddr

```cpp
uint8_t i2cAddr = 0x00;  // 在 instance() 中赋值，boot 时传给硬件库
```

## 现有设备

- [DeviceI2c - OledSsd1306](内置设备-DeviceI2c-OledSsd1306.md)

## 新增 I2C 设备

1. 写头文件：继承 `DeviceI2cBase<Derived, TwoWire>`，声明 `instance()`、`boot()`、`unboot()`
2. 写实现文件：第一行 `#include "core/base/DeviceBase.i2c.cpp"`
3. 在 `EasyBoard.h` 的 `devicesI2c` struct 中挂载
