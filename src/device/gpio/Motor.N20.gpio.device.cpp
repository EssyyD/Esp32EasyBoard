#include "core/base/DeviceBase.gpio.cpp"
#include "Motor.N20.gpio.device.h"

#include <Arduino.h>

// ---- instance() ----
MotorN20Device* MotorN20Device::instance(const char* deviceName, gpio_num_t encAPin, gpio_num_t encBPin) {
    auto* d = new MotorN20Device();
    d->deviceName = deviceName;
    d->PIN_GPIOS.encA.pin = encAPin;
    d->PIN_GPIOS.encB.pin = encBPin;
    return d;
}

// ---- ISR ----
static void IRAM_ATTR _n20EncA_ISR(void* arg) {
    auto* dev = static_cast<MotorN20Device*>(arg);
    if (digitalRead(dev->PIN_GPIOS.encB.pin)) dev->position--;
    else                                       dev->position++;
}

// ---- boot / unboot ----
bool MotorN20Device::boot(const char* serviceName) {
    if (!DeviceGpioBase::boot(serviceName)) return false;

    pinMode(this->PIN_GPIOS.encA.pin, INPUT_PULLUP);
    pinMode(this->PIN_GPIOS.encB.pin, INPUT_PULLUP);

    attachInterruptArg(this->PIN_GPIOS.encA.pin, _n20EncA_ISR, this, CHANGE);

    Serial.printf("[%s] boot OK\n", this->deviceName);
    return true;
}

void MotorN20Device::unboot(const char* serviceName) {
    DeviceGpioBase::unboot(serviceName);
    if (this->bootServices.empty()) {
        detachInterrupt(this->PIN_GPIOS.encA.pin);
    }
}

// ---- encoder ----
long MotorN20Device::getPosition() {
    return this->position;
}

void MotorN20Device::resetPosition() {
    this->position = 0;
}

// ---- driver binding ----
void MotorN20Device::setDriver(DeviceBase* d, int ch, gpio_num_t in1, gpio_num_t in2) {
    this->driver = d;
    this->pwmChannel = ch;
    this->in1Pin = in1;
    this->in2Pin = in2;
}

void MotorN20Device::setSpeed(int s) {
    this->speed = constrain(s, 0, 100);
}

// ---- motion ----
void MotorN20Device::forward() {
    if (!this->driver) return;
    digitalWrite(this->in1Pin, HIGH);
    digitalWrite(this->in2Pin, LOW);
    ledcWrite(this->pwmChannel, map(this->speed, 0, 100, 0, 255));
}

void MotorN20Device::backward() {
    if (!this->driver) return;
    digitalWrite(this->in1Pin, LOW);
    digitalWrite(this->in2Pin, HIGH);
    ledcWrite(this->pwmChannel, map(this->speed, 0, 100, 0, 255));
}

void MotorN20Device::brake() {
    if (!this->driver) return;
    digitalWrite(this->in1Pin, LOW);
    digitalWrite(this->in2Pin, LOW);
    ledcWrite(this->pwmChannel, 0);
}

void MotorN20Device::coast() {
    if (!this->driver) return;
    digitalWrite(this->in1Pin, HIGH);
    digitalWrite(this->in2Pin, HIGH);
    ledcWrite(this->pwmChannel, 0);
}
