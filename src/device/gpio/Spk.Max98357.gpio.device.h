#pragma once
#include "core/base/DeviceBase.gpio.h"

#include <driver/i2s.h>
#include <vector>
#include <cmath>

class SpkMax98357GpioDevice : public DeviceGpioBase<SpkMax98357GpioDevice> {
   public:
    struct Pins {
        PinDef bclk{"Spk.Max98357.BCLK", GPIO_NUM_NC};
        PinDef lrck{"Spk.Max98357.LRCK", GPIO_NUM_NC};
        PinDef dout{"Spk.Max98357.DOUT", GPIO_NUM_NC};
        PinDef* begin() { return &bclk; }
        PinDef* end()   { return &dout + 1; }
    } PIN_GPIOS;

    int sampleRate = 24000;
    int volFactor  = 32112;

    void setDeviceSampleRate(int rate) { this->sampleRate = rate; }
    void setDeviceVolume(int vol);

    static SpkMax98357GpioDevice* instance(const char* deviceName, gpio_num_t bclk, gpio_num_t lrck, gpio_num_t dout);

    bool boot(const char* serviceName);
    bool write(const std::vector<int16_t>& data);
    void unboot(const char* serviceName);

   protected:
    static constexpr bool _MULTI_INSTANCE = false;
};
