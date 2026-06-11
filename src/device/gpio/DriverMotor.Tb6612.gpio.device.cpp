#include "core/base/DeviceBase.gpio.cpp"
#include "DriverMotor.Tb6612.gpio.device.h"

#include <Arduino.h>

// ---- instance() ----
DriverMotorTb6612Device* DriverMotorTb6612Device::instance(
    const char* deviceName,
    gpio_num_t m1PWM, gpio_num_t m1In1, gpio_num_t m1In2,
    gpio_num_t m2PWM, gpio_num_t m2In1, gpio_num_t m2In2,
    gpio_num_t stby
) {
    static DriverMotorTb6612Device inst;
    inst.deviceName = deviceName;
    inst.PIN_GPIOS.motor1PWM.pin = m1PWM;
    inst.PIN_GPIOS.motor1In1.pin = m1In1;
    inst.PIN_GPIOS.motor1In2.pin = m1In2;
    inst.PIN_GPIOS.motor2PWM.pin = m2PWM;
    inst.PIN_GPIOS.motor2In1.pin = m2In1;
    inst.PIN_GPIOS.motor2In2.pin = m2In2;
    inst.PIN_GPIOS.stby.pin = stby;
    return &inst;
}

// ---- boot / unboot ----
bool DriverMotorTb6612Device::boot(const char* serviceName) {
    if (!DeviceGpioBase::boot(serviceName)) return false;

    // motor1 (必接)
    pinMode(this->PIN_GPIOS.motor1PWM.pin, OUTPUT);
    pinMode(this->PIN_GPIOS.motor1In1.pin, OUTPUT);
    pinMode(this->PIN_GPIOS.motor1In2.pin, OUTPUT);
    ledcSetup(0, 20000, 8);
    ledcAttachPin(this->PIN_GPIOS.motor1PWM.pin, 0);

    // motor2 (可选)
    if (this->PIN_GPIOS.motor2PWM.pin != GPIO_NUM_NC) {
        pinMode(this->PIN_GPIOS.motor2PWM.pin, OUTPUT);
        pinMode(this->PIN_GPIOS.motor2In1.pin, OUTPUT);
        pinMode(this->PIN_GPIOS.motor2In2.pin, OUTPUT);
        ledcSetup(1, 20000, 8);
        ledcAttachPin(this->PIN_GPIOS.motor2PWM.pin, 1);
    }

    pinMode(this->PIN_GPIOS.stby.pin, OUTPUT);
    digitalWrite(this->PIN_GPIOS.stby.pin, HIGH);
    digitalWrite(this->PIN_GPIOS.motor1In1.pin, LOW);
    digitalWrite(this->PIN_GPIOS.motor1In2.pin, LOW);
    if (this->PIN_GPIOS.motor2PWM.pin != GPIO_NUM_NC) {
        digitalWrite(this->PIN_GPIOS.motor2In1.pin, LOW);
        digitalWrite(this->PIN_GPIOS.motor2In2.pin, LOW);
    }

    if (this->motors.motor1) {
        this->motors.motor1->setEasyBoard(this->bootEASYB);
        this->motors.motor1->boot(this->deviceName);
        this->motors.motor1->setDriver(this, 0, this->PIN_GPIOS.motor1In1.pin, this->PIN_GPIOS.motor1In2.pin);
    }

    if (this->motors.motor2) {
        this->motors.motor2->setEasyBoard(this->bootEASYB);
        this->motors.motor2->boot(this->deviceName);
        this->motors.motor2->setDriver(this, 1, this->PIN_GPIOS.motor2In1.pin, this->PIN_GPIOS.motor2In2.pin);
    }

    Serial.printf("[%s] boot OK\n", this->deviceName);
    return true;
}

void DriverMotorTb6612Device::unboot(const char* serviceName) {
    DeviceGpioBase::unboot(serviceName);
    if (this->bootServices.empty()) {
        if (this->motors.motor2) this->motors.motor2->unboot(this->deviceName);
        if (this->motors.motor1) this->motors.motor1->unboot(this->deviceName);

        digitalWrite(this->PIN_GPIOS.motor1In1.pin, LOW);
        digitalWrite(this->PIN_GPIOS.motor1In2.pin, LOW);
        ledcDetachPin(this->PIN_GPIOS.motor1PWM.pin);
        if (this->PIN_GPIOS.motor2PWM.pin != GPIO_NUM_NC) {
            digitalWrite(this->PIN_GPIOS.motor2In1.pin, LOW);
            digitalWrite(this->PIN_GPIOS.motor2In2.pin, LOW);
            ledcDetachPin(this->PIN_GPIOS.motor2PWM.pin);
        }
        digitalWrite(this->PIN_GPIOS.stby.pin, LOW);
    }
}

void DriverMotorTb6612Device::standby() {
    digitalWrite(this->PIN_GPIOS.stby.pin, LOW);
}
