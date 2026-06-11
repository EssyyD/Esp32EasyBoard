#include "core/base/DeviceBase.i2c.cpp"
#include "Oled.Ssd1306.i2c.device.h"
#include "core/EasyBoard.h"

// ---- instance() ----
OledSsd1306I2cDevice* OledSsd1306I2cDevice::instance(const char* deviceName, uint8_t i2cAddr) {
    static OledSsd1306I2cDevice inst;
    inst.deviceName = deviceName;
    inst.i2cAddr = i2cAddr;
    return &inst;
}

bool OledSsd1306I2cDevice::boot(const char* serviceName) {
    if (!DeviceI2cBase::boot(serviceName)) return false;

    this->display = new Adafruit_SSD1306(WIDTH, HEIGHT, this->i2c, -1);
    if (!this->display->begin(SSD1306_SWITCHCAPVCC, this->i2cAddr)) return false;
    this->display->clearDisplay();
    this->display->setTextSize(1);
    this->display->setTextColor(SSD1306_WHITE);
    this->display->setCursor(0, 0);
    this->display->println("oled Test");
    this->display->display();
    delay(10);
    return true;
}

void OledSsd1306I2cDevice::unboot(const char* serviceName) {
    DeviceI2cBase::unboot(serviceName);
}

bool OledSsd1306I2cDevice::initI2c() {
    this->i2c = EASYB.i2c0;
    return this->i2c != nullptr;
}

void OledSsd1306I2cDevice::print(const char* text) {
    if (!this->display) return;
    this->display->clearDisplay();
    this->display->setCursor(0, 0);
    this->display->setTextColor(SSD1306_WHITE);
    this->display->println(text);
    this->display->display();
    delay(10);
}

void OledSsd1306I2cDevice::clear() {
    if (!this->display) return;
    this->display->clearDisplay();
    this->display->display();
}
