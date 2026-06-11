#include "DeviceBase.i2c.h"

template<typename Derived, typename BusType>
bool DeviceI2cBase<Derived, BusType>::boot(const char* serviceName) {
    if (!DeviceBase::boot(serviceName)) return false;
    auto* derived = static_cast<Derived*>(this);
    if (!derived->initI2c()) { DeviceBase::unboot(serviceName); return false; }
    return true;
}

template<typename Derived, typename BusType>
void DeviceI2cBase<Derived, BusType>::unboot(const char* serviceName) {
    DeviceBase::unboot(serviceName);
}
