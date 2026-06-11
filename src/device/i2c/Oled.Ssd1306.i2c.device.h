#pragma once
#include "core/base/DeviceBase.i2c.h"
#include <Adafruit_SSD1306.h>
#include <Wire.h>

class OledSsd1306I2cDevice : public DeviceI2cBase<OledSsd1306I2cDevice, TwoWire> {
   public:
    static constexpr int WIDTH  = 128;
    static constexpr int HEIGHT = 32;

    Adafruit_SSD1306* display = nullptr;

    static OledSsd1306I2cDevice* instance(const char* deviceName, uint8_t i2cAddr);

    bool boot(const char* serviceName);
    void unboot(const char* serviceName);

    bool initI2c();

    void print(const char* text);
    void clear();

   protected:
    static constexpr bool _MULTI_INSTANCE = false;
};
