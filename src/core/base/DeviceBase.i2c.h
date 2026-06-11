#pragma once
#include "DeviceBase.h"

template<typename Derived, typename BusType>
class DeviceI2cBase : public DeviceBase {
   public:
    uint8_t i2cAddr = 0x00;
    BusType* i2c = nullptr;

    bool boot(const char* serviceName);
    void unboot(const char* serviceName);
};
