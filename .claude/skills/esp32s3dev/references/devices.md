# 设备层(src/device/)

## 设备清单

| 类 | 文件 | 总线基类 | 多实例 | 关键资源 | EasyBoard 挂载点 |
|---|---|---|---|---|---|
| `MicInmp441GpioDevice` | gpio/Mic.Inmp441 | DeviceGpioBase | 否 | **I2S_NUM_0**(RX) | `devicesGpio.mic` |
| `SpkMax98357GpioDevice` | gpio/Spk.Max98357 | DeviceGpioBase | 否 | **I2S_NUM_1**(TX) | `devicesGpio.spk` |
| `DriverMotorTb6612Device` | gpio/DriverMotor.Tb6612 | DeviceGpioBase | 否 | **LEDC ch0/ch1** 20kHz 8bit | `devicesGpio.tb6612Wheels` |
| `MotorN20Device` | gpio/Motor.N20 | DeviceGpioBase | 是 | 编码器 GPIO 中断 | 挂在 TB6612 的 `motors.motor1/2` |
| `OledSsd1306I2cDevice` | i2c/Oled.Ssd1306 | DeviceI2cBase | 否 | i2c0,地址 0x3C | `devicesI2c.oledSsd13065` |
| `LcdNv3007Device` | spi/Lcd.Nv3007 | DeviceSpiBase | **是** | spiH @50MHz MODE3 | `devicesSpi.lcdNv3007(_2)` |
| `WifiEsp32S3Device` | Wifi.esp32S3(根目录) | DeviceBase 直接子类 | — | WiFi 外设 | `wifi` |

## 各设备要点

### Mic.INMP441(I2S MEMS 麦克风)

- `boot`:i2s_driver_install(I2S_NUM_0, MASTER|RX, 32bit, ONLY_LEFT, dma 8×480)
- `read(std::vector<int16_t>& data, size_t samples)`:读 32bit 原始帧,**右移 12 位**截到 int16 并夹紧;超时 500ms
- `setDeviceSampleRate(rate)` 必须在 boot 之前调
- `unboot` 在最后一个持有者时 `i2s_driver_uninstall(I2S_NUM_0)`

### Spk.MAX98357(I2S 功放)

- `boot`:I2S_NUM_1, MASTER|TX,其余同上
- `setDeviceVolume(0~100)`:音量曲线 `volFactor = (vol/100)² × 65536`
- `write(const std::vector<int16_t>&)`:int16 × volFactor → int32 夹紧后 i2s_write(portMAX_DELAY 阻塞)

### DriverMotor.TB6612(双路电机驱动)

- motor1 必接;motor2 引脚为 NC 时跳过
- PWM:`ledcSetup(0, 20000, 8)` + `ledcSetup(1, ...)`(LEDC 通道 0/1 被此设备占用)
- boot 时若 `motors.motor1/motor2` 已挂 N20 实例,自动级联:`setEasyBoard → boot(this->deviceName) → setDriver(this, ch, in1, in2)`
- N20 实例的挂载发生在 `EasyBoard::onboardDeviceGpio()`
- `standby()` 拉低 STBY;unboot 时停车 + ledcDetachPin + STBY 拉低

### Motor.N20(编码电机)

- 多实例(`new`);构造参数只有编码器 A/B 引脚,动力引脚来自 `setDriver`
- boot:编码器 A 相 `attachInterruptArg(..., CHANGE)`,ISR 按 B 相电平 `position++/--`(IRAM_ATTR)
- API:`setSpeed(0~100)`、`forward/backward/brake/coast`、`getPosition/resetPosition`
- forward/backward 内部 `map(speed,0,100,0,255)` 写 ledcWrite;没挂 driver 时所有动作 no-op

### Oled.SSD1306(0.91" 128×32 I2C)

- `initI2c()` 取 `EASYB.i2c0`;boot 里 `new Adafruit_SSD1306` + begin(0x3C)
- API:`print(text)`(整屏清空后打印)、`clear()`
- 当前 pinGpios 里 OLED 引脚 = NC(实际走 i2c0 总线,地址宏 `PIN_GPIO_OLED_SSD1306_I2C_ADDR`)

### Lcd.NV3007 → 见 display-lvgl.md

### Wifi.esp32S3 → 见 connectivity-web-data.md

## 新增设备步骤(以 SPI 设备 Foo.Bar 为例)

1. **引脚**:在 `config/pinGpios.config.h` 加 `PIN_GPIO_FOO_BAR_CS` 等宏(对照 build-flash.md 的占用表,未接线写 `GPIO_NUM_NC`)。

2. **头文件** `src/device/spi/Foo.Bar.spi.device.h`:

```cpp
#pragma once
#include "core/base/DeviceBase.spi.h"
#include <SPI.h>

class FooBarSpiDevice : public DeviceSpiBase<FooBarSpiDevice, SPIClass> {
   public:
    struct Pins {
        PinDef cs{"Foo.Bar.CS", GPIO_NUM_NC};
        PinDef* begin() { return &cs; }
        PinDef* end()   { return &cs + 1; }
    } PIN_GPIOS;

    static FooBarSpiDevice* instance(const char* deviceName, gpio_num_t cs);

    bool boot(const char* serviceName);
    void unboot(const char* serviceName);
    bool initSpi();          // SPI 设备必须;I2C 设备则为 bool initI2c()

   protected:
    static constexpr bool _MULTI_INSTANCE = false;   // true → instance() 用 new
};
```

3. **实现文件** `src/device/spi/Foo.Bar.spi.device.cpp`:

```cpp
#include "core/base/DeviceBase.spi.cpp"   // ⚠ 第一行:模板实例化,按总线选 .gpio/.i2c/.spi
#include "Foo.Bar.spi.device.h"
#include "core/EasyBoard.h"

FooBarSpiDevice* FooBarSpiDevice::instance(const char* deviceName, gpio_num_t cs) {
    static FooBarSpiDevice inst;          // _MULTI_INSTANCE=true 时改为 new
    inst.deviceName = deviceName;
    inst.PIN_GPIOS.cs.pin = cs;
    return &inst;
}

bool FooBarSpiDevice::boot(const char* serviceName) {
    if (!DeviceSpiBase::boot(serviceName)) return false;   // 非首个持有者,直接复用
    // …… 硬件初始化(只在首个 boot 执行)……
    return true;
}

void FooBarSpiDevice::unboot(const char* serviceName) {
    DeviceSpiBase::unboot(serviceName);
    if (this->bootServices.empty()) { /* 释放硬件 */ }
}

bool FooBarSpiDevice::initSpi() {
    this->spi = EASYB.spiH;               // 永远复用 EasyBoard 的总线
    return this->spi != nullptr;
}
```

4. **挂载**:`core/EasyBoard.h` include 头文件,并在对应分组 struct 里加:

```cpp
struct {
    FooBarSpiDevice* fooBar = FooBarSpiDevice::instance("Foo.Bar", PIN_GPIO_FOO_BAR_CS);
    // …已有设备…
} devicesSpi;
```

5. **GPIO 类设备额外一步**:在 `EasyBoard::onboardDeviceGpio()` 里 `setEasyBoard(this)`(GPIO 基类经 bootEASYB 注册引脚,不注入会空指针)。

6. 在某个服务的 `_rtosBegin/_rtosEnd` 里 boot/unboot 使用它(见 services.md)。

### 各总线差异提醒

- **GPIO 设备**:基类 boot 只注册引脚,`pinMode` 等自己做;别忘了 onboardDeviceGpio 注入。
- **I2C 设备**:确认 `pinGpios.config.h` 的 `PIN_GPIO_I2C_SDA/SCL` 非 NC,否则 i2c0 没 begin。
- **SPI 设备**:基类 boot 已把所有 PIN_GPIOS 置 OUTPUT+HIGH(片选拉高);确认 SPIH/SPIF 引脚非 NC。
- **I2S 设备**:ESP32-S3 只有 I2S_NUM_0(已被 Mic 占)和 I2S_NUM_1(已被 Spk 占),新 I2S 设备需复用或让出。
- **软 I2C 设备**:不走 DeviceBase 体系,挂在 `I2cSoftBase` 子类的 `devices` struct,见 `i2cSofts/I2cSoftExample.h`。
