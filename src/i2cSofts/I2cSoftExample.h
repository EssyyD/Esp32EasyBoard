#pragma once
#include "core/base/I2cSoftBase.h"

// 软件 I2C 总线示例 — 引脚 17/18
class I2cSoftExample : public I2cSoftBase {
   public:
    struct {
        // 将来这里放 I2cSoftBase 上的设备，例如：
        // SomeDevice someSensor;
    } devices;

    void initBus() override {
        this->_sdaGpio = 17;
        this->_sclGpio = 18;
        this->_clock   = 100000;
    }

    void onbusDevices() override {
        // this->devices.someSensor.setI2cBus(this);
    }
};
