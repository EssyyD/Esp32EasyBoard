#pragma once
#include "DeviceBase.h"

template<typename Derived, typename BusType>
class DeviceSpiBase : public DeviceBase {
   public:
    BusType* spi = nullptr;

    bool boot(const char* serviceName);
    void unboot(const char* serviceName);
};
