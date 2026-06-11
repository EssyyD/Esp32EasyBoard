#pragma once
#include "core/base/DeviceBase.gpio.h"

#include <driver/i2s.h>
#include <vector>

class MicInmp441GpioDevice : public DeviceGpioBase<MicInmp441GpioDevice> {
   public:
    struct Pins {
        PinDef sck{"Mic.Inmp441.SCK", GPIO_NUM_NC};
        PinDef ws {"Mic.Inmp441.WS",  GPIO_NUM_NC};
        PinDef din{"Mic.Inmp441.DIN", GPIO_NUM_NC};
        PinDef* begin() { return &sck; }
        PinDef* end()   { return &din + 1; }
    } PIN_GPIOS;

    int sampleRate = 16000;

    void setDeviceSampleRate(int rate) { this->sampleRate = rate; }

    static MicInmp441GpioDevice* instance(const char* deviceName, gpio_num_t sck, gpio_num_t ws, gpio_num_t din);

    bool boot(const char* serviceName);
    bool read(std::vector<int16_t>& data, size_t samples);
    void unboot(const char* serviceName);

   protected:
    static constexpr bool _MULTI_INSTANCE = false;
};
