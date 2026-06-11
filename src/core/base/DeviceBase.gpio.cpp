#include "DeviceBase.gpio.h"
#include "core/EasyBoard.h"

template<typename Derived>
bool DeviceGpioBase<Derived>::boot(const char* serviceName) {
    if (!DeviceBase::boot(serviceName)) return false;
    auto* derived = static_cast<Derived*>(this);
    for (auto& pin : derived->PIN_GPIOS)
        this->bootEASYB->bootGPIO(pin.pin, pin.owner);
    return true;
}

template<typename Derived>
void DeviceGpioBase<Derived>::unboot(const char* serviceName) {
    DeviceBase::unboot(serviceName);
    if (!this->bootServices.empty()) return;
    auto* derived = static_cast<Derived*>(this);
    for (auto& pin : derived->PIN_GPIOS)
        this->bootEASYB->unbootGPIO(pin.pin);
    this->bootEASYB = nullptr;
    Serial.printf("[UNBOOT GPIO] %s  DONE\n", derived->deviceName);
}
