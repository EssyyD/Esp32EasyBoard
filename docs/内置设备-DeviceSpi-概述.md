# 内置设备 DeviceSpi - 概述

SPI 设备继承 `DeviceSpiBase<Derived, BusType>`（CRTP 模板基类）。

## 获取总线

基类已提供默认实现，`this->spi` 自动指向 `EASYB.spiH`。**绝大多数情况下无需重写**。

> ⚠ **严禁在设备中自行 `new SPIClass`。** 框架已在 `EasyBoard::initSpi()` 中创建了唯一的 `SPIClass` 实例（`spiH` / `spiF`），自行 new 会导致冲突。

只有使用非标准 SPI 时才需重写 `initSpi()`。

## 基类自动处理 CS/DC

`DeviceSpiBase::boot()` 自动遍历 `PIN_GPIOS`，将每个引脚设为 `OUTPUT + HIGH`（拉高片选），然后调用 `initSpi()`。子类无需手动处理。

## 现有设备

- [DeviceSpi - LcdNv3007](内置设备-DeviceSpi-LcdNv3007.md)

## 新增 SPI 设备

1. 在 `config/pinGpios.config.h` 定义 CS/DC/RST 引脚宏
2. 写头文件：继承 `DeviceSpiBase<Derived, SPIClass>`，声明 `PINS`、`instance()`、`boot()`、`unboot()`
3. 写实现文件：第一行 `#include "core/base/DeviceBase.spi.cpp"`
4. 在 `EasyBoard.h` 的 `devicesSpi` struct 中挂载
