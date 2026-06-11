#include "Motion.service.rtos.h"
#include "core/EasyBoard.h"

#include <Arduino.h>

void MotionServiceRtos::initRtos() {
    this->rtosTaskName    = this->serviceName;
    this->rtosStackSize   = 4096;
    this->rtosTaskDelayMs = 20;
}

void MotionServiceRtos::_rtosBegin() {
    auto* driver = EASYB.devicesGpio.tb6612Wheels;
    driver->boot(this->serviceName);
    driver->motor1SetSpeed(100);
    Serial.println("[Motion] boot OK");
}

void MotionServiceRtos::_rtosLoop() {
    this->_testPhase  = (this->_testPhase + 1) % 100;
    this->_printCycle = (this->_printCycle + 1) % 10;

    auto* driver = EASYB.devicesGpio.tb6612Wheels;

    if (this->_testPhase < 50)
        driver->motor1Forward();
    else
        driver->motor1Backward();

    if (this->_printCycle == 0)
        Serial.printf("[Motion] phase=%d enc=%ld\n", this->_testPhase,
                      driver->motors.motor1->getPosition());
}

void MotionServiceRtos::_rtosEnd() {
    auto* driver = EASYB.devicesGpio.tb6612Wheels;
    driver->motor1Brake();
    driver->unboot(this->serviceName);
}
