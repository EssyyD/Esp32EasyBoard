#include "Motion.service.rtos.h"
#include "core/EasyBoard.h"

#include <Arduino.h>

void MotionServiceRtos::initRtos() {
    this->rtosTaskName    = this->serviceName;
    this->rtosStackSize   = 4096;
    this->rtosTaskDelayMs = 20;
}

void MotionServiceRtos::_rtosBegin() {
    EASYB.devicesGpio.tb6612Wheels->boot(this->serviceName);
    EASYB.devicesGpio.tb6612Wheels->motors.motor1->setSpeed(100);
    Serial.println("[Motion] boot OK");
}

void MotionServiceRtos::_rtosLoop() {
    this->_testPhase  = (this->_testPhase + 1) % 100;
    this->_printCycle = (this->_printCycle + 1) % 10;

    if (this->_testPhase < 50)
        EASYB.devicesGpio.tb6612Wheels->motors.motor1->forward();
    else
        EASYB.devicesGpio.tb6612Wheels->motors.motor1->backward();

    if (this->_printCycle == 0)
        Serial.printf("[Motion] phase=%d enc=%ld\n", this->_testPhase,
                      EASYB.devicesGpio.tb6612Wheels->motors.motor1->getPosition());
}

void MotionServiceRtos::_rtosEnd() {
    EASYB.devicesGpio.tb6612Wheels->motors.motor1->brake();
    EASYB.devicesGpio.tb6612Wheels->unboot(this->serviceName);
}
