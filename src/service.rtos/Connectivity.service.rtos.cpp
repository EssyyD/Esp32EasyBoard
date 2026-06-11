#include "core/base/DeviceBase.gpio.cpp"
#include "Connectivity.service.rtos.h"
#include "core/EasyBoard.h"

#include <Arduino.h>

void ConnectivityServiceRtos::_rtosBegin() {
    Serial.println("[ConnectivityServiceRtos] begin...");

    EASYB.wifi.boot(this->serviceName);
    Serial.println("[ConnectivityServiceRtos] wifi.boot OK, apOn...");
    EASYB.wifi.apOn();
    Serial.printf(
        "[ConnectivityServiceRtos] AP started, IP=%s\n",
        EASYB.wifi.getApIP().toString().c_str()
    );
}

void ConnectivityServiceRtos::_rtosLoop() {
    
}

void ConnectivityServiceRtos::_rtosEnd() {
    EASYB.wifi.off();
    EASYB.wifi.unboot(this->serviceName);
}
