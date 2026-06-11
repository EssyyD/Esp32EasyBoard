#include "DeviceBase.spi.h"
#include "core/EasyBoard.h"

template<typename Derived, typename BusType>
bool DeviceSpiBase<Derived, BusType>::boot(const char* serviceName) {
    if (!DeviceBase::boot(serviceName)) return false;
    auto* derived = static_cast<Derived*>(this);
    for (auto& pin : derived->PIN_GPIOS) {
        EASYB.bootGPIO(pin.pin, pin.owner);
        pinMode(pin.pin, OUTPUT);
        digitalWrite(pin.pin, HIGH);
    }
    if (!derived->initSpi()) { DeviceBase::unboot(serviceName); return false; }
    return true;
}

template<typename Derived, typename BusType>
void DeviceSpiBase<Derived, BusType>::unboot(const char* serviceName) {
    DeviceBase::unboot(serviceName);
    if (!this->bootServices.empty()) return;
    auto* derived = static_cast<Derived*>(this);
    for (auto& pin : derived->PIN_GPIOS)
        EASYB.unbootGPIO(pin.pin);
}
