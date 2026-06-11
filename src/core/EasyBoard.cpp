#include "EasyBoard.h"
#include <LittleFS.h>

EasyBoard EasyBoard::BOARD;

EasyBoard::EasyBoard() {}

void EasyBoard::init() {
    delay(800);
    Serial.begin(115200);
    LittleFS.begin(false);
    this->bootGPIO(PIN_GPIO_LED_BUILTIN, "NeoPixel");
    this->neoPixel.begin();
    this->neoPixel.setBrightness(32);
    this->neoPixel.clear();
    this->neoPixel.show();
    this->initI2c();
    this->initSpi();
    this->onboardDeviceGpio();
}

// I2C0 初始化 —— 项目中 Wire.begin() 的唯一位置
// 其他地方禁止 Wire.begin()，否则 EasyBoard.i2c0 将失效
void EasyBoard::initI2c() {
    if (PIN_GPIO_I2C_SDA != GPIO_NUM_NC && PIN_GPIO_I2C_SCL != GPIO_NUM_NC) {
        Wire.begin(PIN_GPIO_I2C_SDA, PIN_GPIO_I2C_SCL);
        Wire.setClock(400000);
        Serial.printf("[EasyBoard] Wire.begin(SDA=%d, SCL=%d)\n", PIN_GPIO_I2C_SDA, PIN_GPIO_I2C_SCL);
    }
}

// SPI 主机初始化 —— 项目中 new SPIClass 的唯一位置
// 其他地方禁止 new SPIClass，否则 EasyBoard.spiH/spiF 将失效
void EasyBoard::initSpi() {
    if (PIN_GPIO_SPIH_SCK != GPIO_NUM_NC && PIN_GPIO_SPIH_MOSI != GPIO_NUM_NC) {
        this->spiH = new SPIClass(HSPI);
        this->spiH->begin(PIN_GPIO_SPIH_SCK, PIN_GPIO_SPIH_MISO, PIN_GPIO_SPIH_MOSI);
        Serial.printf("[EasyBoard] HSPI begin(SCK=%d, MOSI=%d)\n", PIN_GPIO_SPIH_SCK, PIN_GPIO_SPIH_MOSI);
    }
    if (PIN_GPIO_SPIF_SCK != GPIO_NUM_NC && PIN_GPIO_SPIF_MOSI != GPIO_NUM_NC) {
        this->spiF = new SPIClass(FSPI);
        this->spiF->begin(PIN_GPIO_SPIF_SCK, PIN_GPIO_SPIF_MISO, PIN_GPIO_SPIF_MOSI);
        Serial.printf("[EasyBoard] FSPI begin(SCK=%d, MOSI=%d)\n", PIN_GPIO_SPIF_SCK, PIN_GPIO_SPIF_MOSI);
    }
}

// 设备上板
void EasyBoard::onboardDeviceGpio() {
    // -- attach motor instances to drivers --
    this->devicesGpio.tb6612Wheels->motors.motor1 = MotorN20Device::instance(
        "Motor.N20.Wheel1",
        PIN_GPIO_MOTOR_N20_WHEEL_1_ENC_A,
        PIN_GPIO_MOTOR_N20_WHEEL_1_ENC_B
    );

    // -- inject EasyBoard --
    this->devicesGpio.mic->setEasyBoard(this);
    this->devicesGpio.spk->setEasyBoard(this);
    this->devicesGpio.tb6612Wheels->setEasyBoard(this);
}

void EasyBoard::setLed(uint8_t r, uint8_t g, uint8_t b) {
    this->neoPixel.setPixelColor(0, this->neoPixel.Color(r, g, b));
    this->neoPixel.show();
}

bool EasyBoard::bootGPIO(gpio_num_t pin, const char* owner) {
    if (pin == GPIO_NUM_NC) return true;
    for (int i = 0; i < this->pinsAllocCount; i++) {
        if (this->pinsAlloc[i].pin == pin) {
            Serial.printf(
                "[GPIO] CONFLICT: %d already claimed by '%s', now '%s'\n",
                pin, this->pinsAlloc[i].owner, owner
            );
            return false;
        }
    }
    if (this->pinsAllocCount >= MAX_PINS) return false;
    this->pinsAlloc[this->pinsAllocCount++] = {pin, owner};
    return true;
}

void EasyBoard::unbootGPIO(gpio_num_t pin) {
    if (pin == GPIO_NUM_NC) return;
    for (int i = 0; i < this->pinsAllocCount; i++) {
        if (this->pinsAlloc[i].pin == pin) {
            this->pinsAlloc[i] = this->pinsAlloc[--this->pinsAllocCount];
            return;
        }
    }
}

void EasyBoard::dumpPins() {
    Serial.println("=== GPIO Allocations ===");
    for (int i = 0; i < this->pinsAllocCount; i++)
        Serial.printf("  GPIO %2d -> %s\n", this->pinsAlloc[i].pin, this->pinsAlloc[i].owner);
}
