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

    /// @param encAPin / encBPin  编码器引脚，无霍尔编码器时默认 GPIO_NUM_NC
    static MotorN20Device* instance(const char* deviceName,
          gpio_num_t encAPin = GPIO_NUM_NC, gpio_num_t encBPin = GPIO_NUM_NC);

    volatile long position = 0;

    bool boot(const char* serviceName);
    void unboot(const char* serviceName);

    long getPosition();
    void resetPosition();

   protected:
    static constexpr bool _MULTI_INSTANCE = true;
};
