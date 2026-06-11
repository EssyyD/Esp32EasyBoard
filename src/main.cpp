#include <Arduino.h>
#include "core/EasyBoard.h"

void setup() {
    EASYB.init();
    EASYB.dumpPins();

    // EASYB.servicesRtos.voiceEchoServiceRtos.rtosRun();
    // EASYB.servicesRtos.connectivityServiceRtos.rtosRun();
    EASYB.servicesRtos.motionServiceRtos.rtosRun();

    EASYB.servicesRtos.displayServiceRtos.rtosRun();

    Serial.println("[main] done");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
