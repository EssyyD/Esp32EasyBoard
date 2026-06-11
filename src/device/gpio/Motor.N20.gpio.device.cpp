#include "core/base/DeviceBase.gpio.cpp"
#include "Motor.N20.gpio.device.h"

#include <Arduino.h>

// ---- instance() ----
MotorN20Device* MotorN20Device::instance(const char* deviceName,
      gpio_num_t encAPin, gpio_num_t encBPin) {
    auto* d = new MotorN20Device();
    d->deviceName = deviceName;
    d->PIN_GPIOS.encA.pin = encAPin;
    d->PIN_GPIOS.encB.pin = encBPin;
    return d;
}

// ---- ISR ----
static void IRAM_ATTR _n20EncA_ISR(void* arg) {
    auto* dev = static_cast<MotorN20Device*>(arg);
    if (digitalRead(dev->PIN_GPIOS.encB.pin)) {
        dev->position--;
    } else {
        dev->position++;
    }
}

// ---- boot / unboot ----
bool MotorN20Device::boot(const char* serviceName) {
    if (!DeviceGpioBase::boot(serviceName)) return false;

    if (this->PIN_GPIOS.encA.pin != GPIO_NUM_NC) {
        pinMode(this->PIN_GPIOS.encA.pin, INPUT_PULLUP);
        pinMode(this->PIN_GPIOS.encB.pin, INPUT_PULLUP);
        attachInterruptArg(this->PIN_GPIOS.encA.pin, _n20EncA_ISR, this, CHANGE);
    }

    Serial.printf("[%s] boot OK\n", this->deviceName);
    return true;
}

void MotorN20Device::unboot(const char* serviceName) {
    DeviceGpioBase::unboot(serviceName);
    if (this->bootServices.empty()) {
        if (this->PIN_GPIOS.encA.pin != GPIO_NUM_NC) {
            detachInterrupt(this->PIN_GPIOS.encA.pin);
        }
    }
}

// ---- encoder ----
long MotorN20Device::getPosition() {
    return this->position;
}

void MotorN20Device::resetPosition() {
    this->position = 0;
}
