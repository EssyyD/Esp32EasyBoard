#pragma once
#include "config/config.h"
#include "device/gpio/Mic.Inmp441.gpio.device.h"
#include "device/gpio/Spk.Max98357.gpio.device.h"
#include "device/gpio/DriverMotor.Tb6612.gpio.device.h"
#include "device/Wifi.esp32S3.device.h"
#include "device/spi/Lcd.Nv3007.spi.device.h"
#include "device/i2c/Oled.Ssd1306.i2c.device.h"
#include "i2cSofts/I2cSoftExample.h"
#include "core/ChipWebServer.rtos.h"
#include "core/ChipSqlite.h"
#include "service.rtos/VoiceEcho.service.rtos.h"
#include "service.rtos/Connectivity.service.rtos.h"
#include "service.rtos/Motion.service.rtos.h"
#include "service.rtos/Display.service.rtos.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

class EasyBoard {
   public:
    static constexpr int MAX_PINS  = 32;

    static EasyBoard BOARD;

    // SPI 主机实例 —— 项目中唯一入口，禁止在其他任何地方 new SPIClass
    // 否则 EasyBoard 持有的这两个引用将失效
    SPIClass* spiH = nullptr;
    SPIClass* spiF = nullptr;

    // I2C0 实例 —— 项目中唯一入口，禁止在其他任何地方 Wire.begin()
    TwoWire* i2c0 = &Wire;

    WifiEsp32S3Device wifi;

    struct {
        OledSsd1306I2cDevice* oledSsd13065 = OledSsd1306I2cDevice::instance("Oled.SSD1306", PIN_GPIO_OLED_SSD1306_I2C_ADDR);
    } devicesI2c;

    struct {
        LcdNv3007Device*   lcdNv3007   = LcdNv3007Device::instance("Lcd.NV3007",   PIN_GPIO_LCD_NV3007_CS,   PIN_GPIO_LCD_NV3007_DC,   PIN_GPIO_LCD_NV3007_RST,   0);
        LcdNv3007Device*   lcdNv3007_2 = LcdNv3007Device::instance("Lcd.NV3007_2", PIN_GPIO_LCD_NV3007_2_CS, PIN_GPIO_LCD_NV3007_2_DC, PIN_GPIO_LCD_NV3007_2_RST, LcdNv3007Device::ROTATION1_OFFSET);
    } devicesSpi;

    struct {
        // I2cSoftExample i2cSoftExample;
    } i2cSofts;

    struct {
        MicInmp441GpioDevice*          mic          = MicInmp441GpioDevice::instance("Mic.INMP441", PIN_GPIO_MIC_INMP441_SCK, PIN_GPIO_MIC_INMP441_WS, PIN_GPIO_MIC_INMP441_DIN);
        SpkMax98357GpioDevice*         spk          = SpkMax98357GpioDevice::instance("Spk.MAX98357", PIN_GPIO_SPK_MAX98357_BCLK, PIN_GPIO_SPK_MAX98357_LRCK, PIN_GPIO_SPK_MAX98357_DOUT);
        DriverMotorTb6612Device* tb6612Wheels = DriverMotorTb6612Device::instance(
            "Driver.TB6612.Wheels",
            PIN_GPIO_DRIVER_TB6612_WHEEL_W1_PWM, PIN_GPIO_DRIVER_TB6612_WHEEL_W1_IN1, PIN_GPIO_DRIVER_TB6612_WHEEL_W1_IN2,
            PIN_GPIO_DRIVER_TB6612_WHEEL_W2_PWM, PIN_GPIO_DRIVER_TB6612_WHEEL_W2_IN1, PIN_GPIO_DRIVER_TB6612_WHEEL_W2_IN2,
            PIN_GPIO_DRIVER_TB6612_WHEEL_STBY
        );
    } devicesGpio;

    struct {
        VoiceEchoServiceRtos    voiceEchoServiceRtos;
        ConnectivityServiceRtos connectivityServiceRtos;
        MotionServiceRtos       motionServiceRtos;
        DisplayServiceRtos      displayServiceRtos;
    } servicesRtos;

    ChipWebServerRtos chipWebServerRtos;
    ChipSqlite        chipSqlite;

    Adafruit_NeoPixel neoPixel{1, PIN_GPIO_LED_BUILTIN, NEO_GRB + NEO_KHZ800};

    int pinsAllocCount = 0;
    struct { gpio_num_t pin; const char* owner; } pinsAlloc[MAX_PINS];

    static EasyBoard& instance() { return BOARD; }

    EasyBoard();

    void init();
    void initI2c();
    void initSpi();
    void onboardDeviceGpio();
    void setLed(uint8_t r, uint8_t g, uint8_t b);

    bool bootGPIO(gpio_num_t pin, const char* owner);
    void unbootGPIO(gpio_num_t pin);
    void dumpPins();
};

#define EASYB EasyBoard::instance()
