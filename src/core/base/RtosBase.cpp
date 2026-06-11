#include "core/base/RtosBase.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>

void RtosBase::_rtosTask(void* arg) {
    auto* self = static_cast<RtosBase*>(arg);
    self->_rtosBeforeLoopOnce();
    while (self->_rtosRunning) {
        self->_rtosLoop();
        vTaskDelay(pdMS_TO_TICKS(self->rtosTaskDelayMs));
    }
    self->_rtosEnd();
    self->_rtosRunning = false;
    vTaskDelete(nullptr);
}

void RtosBase::rtosRun() {
    this->initRtos();
    if (this->_rtosRunning) {
        Serial.printf("[WARN] %s already running\n", this->rtosTaskName);
        return;
    }
    this->_rtosBegin();
    this->_rtosRunning = true;
    xTaskCreate(
        _rtosTask, this->rtosTaskName, this->rtosStackSize, this, 1, nullptr
    );
    Serial.printf("[RTOS] %s started, stack=%u\n", this->rtosTaskName, this->rtosStackSize);
}

void RtosBase::rtosStop() {
    this->_rtosEnd();
    this->_rtosRunning = false;
}

void RtosBase::serviceDelay(uint32_t ms) {
    this->_delayHook(ms);
    vTaskDelay(pdMS_TO_TICKS(ms));
}
