#pragma once
#include "core/base/DeviceBase.gpio.h"

class MotorN20Device : public DeviceGpioBase<MotorN20Device> {
   public:
    struct Pins {
        PinDef encA{"Motor.N20.EncA", GPIO_NUM_NC};
        PinDef encB{"Motor.N20.EncB", GPIO_NUM_NC};
        PinDef* begin() { return &encA; }
        PinDef* end()   { return &encB + 1; }
    } PIN_GPIOS;

    /// @param deviceName 设备名，如 "Motor.N20.L1"
    /// @param encAPin    编码器 A 相 GPIO
    /// @param encBPin    编码器 B 相 GPIO
    static MotorN20Device* instance(const char* deviceName, gpio_num_t encAPin, gpio_num_t encBPin);

    volatile long position = 0;

    bool boot(const char* serviceName);
    void unboot(const char* serviceName);

    long getPosition();
    void resetPosition();

    DeviceBase* driver = nullptr;
    int pwmChannel = 0;
    int speed = 0;
    gpio_num_t in1Pin = GPIO_NUM_NC;
    gpio_num_t in2Pin = GPIO_NUM_NC;

    void setDriver(DeviceBase* d, int ch, gpio_num_t in1, gpio_num_t in2);
    void setSpeed(int s);
    void forward();
    void backward();
    void brake();
    void coast();

   protected:
    static constexpr bool _MULTI_INSTANCE = true;
};
