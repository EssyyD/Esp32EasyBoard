#include "core/base/DeviceBase.spi.cpp"
#include "Lcd.Nv3007.spi.device.h"
#include "core/EasyBoard.h"

#include <SPI.h>

// ---- instance() ----
LcdNv3007Device* LcdNv3007Device::instance(const char* deviceName, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst,
                                            int32_t hCasetOffset) {
    auto* d = new LcdNv3007Device();
    d->deviceName = deviceName;
    d->PIN_GPIOS.cs.pin = cs;
    d->PIN_GPIOS.dc.pin = dc;
    d->PIN_GPIOS.rst.pin = rst;
    d->hCasetOffset = hCasetOffset;
    return d;
}

bool LcdNv3007Device::boot(const char* serviceName) {
    if (!DeviceSpiBase::boot(serviceName)) return false;

    Serial.println("[NV3007] boot OK");
    return true;
}

void LcdNv3007Device::unboot(const char* serviceName) {
    this->spi = nullptr;
    DeviceSpiBase::unboot(serviceName);
}

bool LcdNv3007Device::initSpi() {
    this->spi = EASYB.spiH;
    Serial.println("[NV3007] initSpi OK");
    return true;
}

void LcdNv3007Device::_spiBegin() { this->spi->beginTransaction(_spiSet); digitalWrite(this->PIN_GPIOS.dc.pin, HIGH); digitalWrite(this->PIN_GPIOS.cs.pin, LOW); }
void LcdNv3007Device::_spiEnd()   { digitalWrite(this->PIN_GPIOS.cs.pin, HIGH); this->spi->endTransaction(); }
void LcdNv3007Device::_spiCmd(uint8_t c)      { digitalWrite(this->PIN_GPIOS.dc.pin, LOW); this->spi->transfer(c); digitalWrite(this->PIN_GPIOS.dc.pin, HIGH); }
void LcdNv3007Device::_spiC8D8(uint8_t c, uint8_t d)   { _spiCmd(c); this->spi->transfer(d); }
void LcdNv3007Device::_spiC8D16(uint8_t c, uint16_t d) { _spiCmd(c); this->spi->write16(d); }
void LcdNv3007Device::_spiWritePixels(const uint16_t* px, size_t n) { while (n--) { uint16_t v = *px++; this->spi->transfer(v >> 8); this->spi->transfer(v & 0xFF); } }

void LcdNv3007Device::_tftInit() {
    if (this->PIN_GPIOS.rst.pin != GPIO_NUM_NC) {
        pinMode(this->PIN_GPIOS.rst.pin, OUTPUT);
        digitalWrite(this->PIN_GPIOS.rst.pin, HIGH); delay(50);
        digitalWrite(this->PIN_GPIOS.rst.pin, LOW);  delay(50);
        digitalWrite(this->PIN_GPIOS.rst.pin, HIGH); delay(120);
    }

    _spiBegin();

    _spiC8D8(0xFF, 0xA5);
    _spiC8D8(0x9A, 0x08); _spiC8D8(0x9B, 0x08);
    _spiC8D8(0x9C, 0xB0); _spiC8D8(0x9D, 0x16);
    _spiC8D8(0x9E, 0xC4); _spiC8D16(0x8F, 0x0455);
    _spiC8D8(0x84, 0x90); _spiC8D8(0x83, 0x7B);
    _spiC8D8(0x85, 0x33);

    uint8_t g[] = {
        0x60,0x00,0x70,0x00,0x61,0x02,0x71,0x02,0x62,0x04,0x72,0x04,
        0x6C,0x29,0x7C,0x29,0x6D,0x31,0x7D,0x31,0x6E,0x0F,0x7E,0x0F,
        0x66,0x21,0x76,0x21,0x68,0x3A,0x78,0x3A,0x63,0x07,0x73,0x07,
        0x64,0x05,0x74,0x05,0x65,0x02,0x75,0x02,0x67,0x23,0x77,0x23,
        0x69,0x08,0x79,0x08,0x6A,0x13,0x7A,0x13,0x6B,0x13,0x7B,0x13,
        0x6F,0x00,0x7F,0x00,
    };
    for (size_t i = 0; i < sizeof(g); i += 2)
        _spiC8D8(g[i], g[i + 1]);

    _spiC8D8(0x50, 0x00); _spiC8D8(0x52, 0xD6);
    _spiC8D8(0x53, 0x08); _spiC8D8(0x54, 0x08);
    _spiC8D8(0x55, 0x1E); _spiC8D8(0x56, 0x1C);

    _spiCmd(0xA0); this->spi->transfer(0x2B); this->spi->transfer(0x24); this->spi->transfer(0x00);
    _spiC8D8(0xA1, 0x87); _spiC8D8(0xA2, 0x86);
    _spiC8D8(0xA5, 0x00); _spiC8D8(0xA6, 0x00);
    _spiC8D8(0xA7, 0x00); _spiC8D8(0xA8, 0x36);
    _spiC8D8(0xA9, 0x7E); _spiC8D8(0xAA, 0x7E);

    _spiC8D8(0xB9, 0x85); _spiC8D8(0xBA, 0x84);
    _spiC8D8(0xBB, 0x83); _spiC8D8(0xBC, 0x82);
    _spiC8D8(0xBD, 0x81); _spiC8D8(0xBE, 0x80);
    _spiC8D8(0xBF, 0x01); _spiC8D8(0xC0, 0x02);
    _spiC8D8(0xC1, 0x00); _spiC8D8(0xC2, 0x00);
    _spiC8D8(0xC3, 0x00); _spiC8D8(0xC4, 0x33);
    _spiC8D8(0xC5, 0x7E); _spiC8D8(0xC6, 0x7E);
    _spiC8D16(0xC8, 0x3333); _spiC8D8(0xC9, 0x68);
    _spiC8D8(0xCA, 0x69); _spiC8D8(0xCB, 0x6A);
    _spiC8D8(0xCC, 0x6B); _spiC8D16(0xCD, 0x3333);
    _spiC8D8(0xCE, 0x6C); _spiC8D8(0xCF, 0x6D);
    _spiC8D8(0xD0, 0x6E); _spiC8D8(0xD1, 0x6F);
    _spiC8D16(0xAB, 0x0367); _spiC8D16(0xAC, 0x036B);
    _spiC8D16(0xAD, 0x0368); _spiC8D16(0xAE, 0x036C);
    _spiC8D8(0xB3, 0x00); _spiC8D8(0xB4, 0x00);
    _spiC8D8(0xB5, 0x00); _spiC8D8(0xB6, 0x32);
    _spiC8D8(0xB7, 0x7E); _spiC8D8(0xB8, 0x7E);
    _spiC8D8(0xE0, 0x00); _spiC8D16(0xE1, 0x030F);
    _spiC8D8(0xE2, 0x04); _spiC8D8(0xE3, 0x01);
    _spiC8D8(0xE4, 0x0E); _spiC8D8(0xE5, 0x01);
    _spiC8D8(0xE6, 0x19); _spiC8D8(0xE7, 0x10);
    _spiC8D8(0xE8, 0x10); _spiC8D8(0xEA, 0x12);
    _spiC8D8(0xEB, 0xD0); _spiC8D8(0xEC, 0x04);
    _spiC8D8(0xED, 0x07); _spiC8D8(0xEE, 0x07);
    _spiC8D8(0xEF, 0x09); _spiC8D8(0xF0, 0xD0);
    _spiC8D8(0xF1, 0x0E);

    _spiC8D8(0xF9, 0x17);
    _spiCmd(0xF2); this->spi->transfer(0x2C); this->spi->transfer(0x1B); this->spi->transfer(0x0B); this->spi->transfer(0x20);
    _spiC8D8(0xE9, 0x29); _spiC8D8(0xEC, 0x04);

    _spiC8D8(0x35, 0x00); _spiC8D16(0x44, 0x0010);
    _spiC8D8(0x46, 0x10);

    _spiC8D8(0xFF, 0x00); _spiC8D8(0x3A, 0x05);
    _spiC8D8(0x36, 0x00);

    _spiEnd();

    _spiBegin();
    _spiCmd(0x11);
    _spiEnd();
    delay(220);

    _spiBegin();
    _spiCmd(0x29);
    _spiEnd();
    delay(200);
}

void LcdNv3007Device::initLcdV() {
    this->_tftInit();
    this->width = SHORT_SIDE;
    this->height = LONG_SIDE;
    this->coordMode = 'V';
    this->pointLeftTop = {0, 0};
    this->pointRightTop = {uint16_t(SHORT_SIDE - 1), 0};
    this->pointRightBottom = {uint16_t(SHORT_SIDE - 1), uint16_t(LONG_SIDE - 1)};
    this->pointLeftBottom = {0, uint16_t(LONG_SIDE - 1)};
    this->pointCenter = {uint16_t(SHORT_SIDE / 2), uint16_t(LONG_SIDE / 2)};
    Serial.printf(
        "[NV3007] initLcdV: portrait %d×%d maxXY=(%d,%d)\n",
        SHORT_SIDE,
        LONG_SIDE,
        SHORT_SIDE - 1,
        LONG_SIDE - 1
    );
}

void LcdNv3007Device::initLcdH() {
    this->_tftInit();
    _spiBegin();
    _spiC8D8(0x36, 0xB0);
    _spiEnd();
    this->width = LONG_SIDE;
    this->height = SHORT_SIDE;
    this->coordMode = 'H';
    this->pointLeftTop = {0, 0};
    this->pointRightTop = {uint16_t(LONG_SIDE - 1), 0};
    this->pointRightBottom = {uint16_t(LONG_SIDE - 1), uint16_t(SHORT_SIDE - 1)};
    this->pointLeftBottom = {0, uint16_t(SHORT_SIDE - 1)};
    this->pointCenter = {uint16_t(LONG_SIDE / 2), uint16_t(SHORT_SIDE / 2)};
    Serial.printf(
        "[NV3007] initLcdH: landscape %d×%d maxXY=(%d,%d)\n",
        LONG_SIDE,
        SHORT_SIDE,
        LONG_SIDE - 1,
        SHORT_SIDE - 1
    );
}

void LcdNv3007Device::flushWrite(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t* pxMap, size_t len) {
    int32_t caOffset = this->flushCasetOffset();
    int32_t raOffset = this->flushRasetOffset();
    _spiBegin();
    _spiCmd(0x2A);
    this->spi->write16(x1 + caOffset);
    this->spi->write16(x2 + caOffset);
    _spiCmd(0x2B);
    this->spi->write16(y1 + raOffset);
    this->spi->write16(y2 + raOffset);
    _spiCmd(0x2C);
    _spiWritePixels(pxMap, len);
    _spiEnd();
}

