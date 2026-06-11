#pragma once
#include <Arduino.h>
#include <vector>

// 软件 I2C 基类 — 用 GPIO 模拟 I2C 时序，不依赖硬件 I2C 控制器
class I2cSoftBase {
   public:
    bool booted = false;

    virtual void initBus() = 0;

    virtual bool boot() {
        if (!this->_busInited) { this->initBus(); this->_busInited = true; }
        if (this->booted) return true;
        pinMode(this->_sdaGpio, OUTPUT_OPEN_DRAIN);
        pinMode(this->_sclGpio, OUTPUT_OPEN_DRAIN);
        digitalWrite(this->_sdaGpio, HIGH);
        digitalWrite(this->_sclGpio, HIGH);
        this->_halfPeriod = (1000000 / this->_clock) / 2;
        if (this->_halfPeriod < 1) this->_halfPeriod = 1;
        this->booted = true;
        Serial.printf("[I2cSoft] begin(SDA=%d, SCL=%d, CLK=%d)\n", this->_sdaGpio, this->_sclGpio, this->_clock);
        this->onbusDevices();
        return true;
    }

    virtual void onbusDevices() {}

    virtual void unboot() {
        pinMode(this->_sdaGpio, INPUT);
        pinMode(this->_sclGpio, INPUT);
        this->booted = false;
    }

    struct { } devices;

    virtual void beginTransmission(uint8_t addr) {
        _start();
        _writeByte(addr << 1);
    }
    virtual uint8_t endTransmission() {
        _stop();
        return 0;
    }
    virtual size_t write(uint8_t v) {
        return _writeByte(v) ? 1 : 0;
    }
    virtual size_t write(const uint8_t* buf, size_t len) {
        size_t n = 0;
        while (n < len && _writeByte(buf[n])) n++;
        return n;
    }
    virtual uint8_t requestFrom(uint8_t addr, size_t len) {
        _start();
        _writeByte((addr << 1) | 1);
        _rxBuf.clear();
        for (size_t i = 0; i < len; i++)
            _rxBuf.push_back(_readByte(i + 1 < len));
        _stop();
        return _rxBuf.size();
    }
    virtual int available() { return _rxBuf.size() - _rxPos; }
    virtual int read()     { return (_rxPos < _rxBuf.size()) ? _rxBuf[_rxPos++] : -1; }

   protected:
    int _sdaGpio = -1;
    int _sclGpio = -1;
    uint32_t _clock = 400000;

   private:
    bool _busInited = false;
    int _halfPeriod = 5;
    std::vector<uint8_t> _rxBuf;
    size_t _rxPos = 0;

    void _sdaHigh() { digitalWrite(_sdaGpio, HIGH); }
    void _sdaLow()  { digitalWrite(_sdaGpio, LOW); }
    void _sclHigh() { digitalWrite(_sclGpio, HIGH); }
    void _sclLow()  { digitalWrite(_sclGpio, LOW); }
    bool _sdaRead() { return digitalRead(_sdaGpio); }
    void _delay()   { delayMicroseconds(_halfPeriod); }

    void _start() {
        _sdaHigh(); _sclHigh(); _delay();
        _sdaLow();  _delay();
        _sclLow();
    }
    void _stop() {
        _sdaLow();  _sclHigh(); _delay();
        _sdaHigh(); _delay();
    }
    bool _writeByte(uint8_t data) {
        for (int i = 7; i >= 0; i--) {
            if (data & (1 << i)) _sdaHigh(); else _sdaLow();
            _delay();
            _sclHigh(); _delay();
            _sclLow();
        }
        _sdaHigh(); _delay();
        _sclHigh();
        bool ack = !_sdaRead();
        _delay();
        _sclLow();
        return ack;
    }
    uint8_t _readByte(bool ack) {
        uint8_t data = 0;
        _sdaHigh();
        for (int i = 7; i >= 0; i--) {
            _sclHigh(); _delay();
            if (_sdaRead()) data |= (1 << i);
            _sclLow(); _delay();
        }
        if (ack) _sdaLow(); else _sdaHigh();
        _delay();
        _sclHigh(); _delay();
        _sclLow();
        _sdaHigh();
        return data;
    }
};
