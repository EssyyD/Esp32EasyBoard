#pragma once
#include "core/base/DeviceBase.spi.h"
#include <SPI.h>

class LcdNv3007Device : public DeviceSpiBase<LcdNv3007Device, SPIClass> {
   public:
    struct Vertex { uint16_t x, y; };

    static constexpr int LONG_SIDE  = 428;
    static constexpr int SHORT_SIDE = 142;
    static constexpr int ROTATION0_OFFSET = 12;
    static constexpr int ROTATION1_OFFSET = 14;

    struct Pins {
        PinDef cs {"Lcd.NV3007.CS",  GPIO_NUM_NC};
        PinDef dc {"Lcd.NV3007.DC",  GPIO_NUM_NC};
        PinDef rst{"Lcd.NV3007.RST", GPIO_NUM_NC};
        PinDef* begin() { return &cs; }
        PinDef* end()   { return &rst + 1; }
    } PIN_GPIOS;

    /// @param hCasetOffset flushCasetOffset 在横屏模式下的偏移量（NV3007=0, NV3007_2=ROTATION1_OFFSET）
    static LcdNv3007Device* instance(const char* deviceName, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst,
                                     int32_t hCasetOffset = 0);

    int32_t hCasetOffset = 0;

    Vertex pointLeftTop = {0, 0};
    Vertex pointRightTop = {0, 0};
    Vertex pointRightBottom = {0, 0};
    Vertex pointLeftBottom = {0, 0};
    Vertex pointCenter = {0, 0};

    uint16_t width  = 0;
    uint16_t height = 0;
    char coordMode = 'V';

    bool boot(const char* serviceName);
    void unboot(const char* serviceName);

    bool initSpi();
    void initLcdV();
    void initLcdH();

    void flushWrite(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t* pxMap, size_t len);
    int32_t flushCasetOffset() const {
        return (this->coordMode == 'H') ? this->hCasetOffset : ROTATION0_OFFSET;
    }
    int32_t flushRasetOffset() const {
        return (this->coordMode == 'H') ? ROTATION0_OFFSET : 0;
    }

   private:
    SPISettings _spiSet{50000000, MSBFIRST, SPI_MODE3};

    void _spiBegin();
    void _spiEnd();
    void _spiCmd(uint8_t c);
    void _spiC8D8(uint8_t c, uint8_t d);
    void _spiC8D16(uint8_t c, uint16_t d);
    void _spiWritePixels(const uint16_t* px, size_t n);

    void _tftInit();

   protected:
    static constexpr bool _MULTI_INSTANCE = true;
};
