#pragma once
#include "core/base/DeviceBase.gpio.h"
#include "Motor.N20.gpio.device.h"

class DriverMotorTb6612Device : public DeviceGpioBase<DriverMotorTb6612Device> {
   public:
    struct Pins {
        PinDef motor1PWM{"Driver.Tb6612.Motor1.PWM", GPIO_NUM_NC};
        PinDef motor1In1{"Driver.Tb6612.Motor1.IN1", GPIO_NUM_NC};
        PinDef motor1In2{"Driver.Tb6612.Motor1.IN2", GPIO_NUM_NC};
        PinDef motor2PWM{"Driver.Tb6612.Motor2.PWM", GPIO_NUM_NC};
        PinDef motor2In1{"Driver.Tb6612.Motor2.IN1", GPIO_NUM_NC};
        PinDef motor2In2{"Driver.Tb6612.Motor2.IN2", GPIO_NUM_NC};
        PinDef stby{"Driver.Tb6612.STBY",             GPIO_NUM_NC};
        PinDef* begin() { return &motor1PWM; }
        PinDef* end()   { return &stby + 1; }
    } PIN_GPIOS;

    struct {
        MotorN20Device* motor1 = nullptr;
        MotorN20Device* motor2 = nullptr;
    } motors;

    static DriverMotorTb6612Device* instance(
        const char* deviceName,
        gpio_num_t m1PWM, gpio_num_t m1In1, gpio_num_t m1In2,
        gpio_num_t m2PWM, gpio_num_t m2In1, gpio_num_t m2In2,
        gpio_num_t stby
    );

    bool boot(const char* serviceName);
    void unboot(const char* serviceName);

    void standby();

    // motor1
    void motor1SetSpeed(uint8_t speed);  // 0~100
    void motor1Forward();
    void motor1Backward();
    void motor1Brake();
    void motor1Coast();

    // motor2（引脚为 NC 时 no-op）
    void motor2SetSpeed(uint8_t speed);  // 0~100
    void motor2Forward();
    void motor2Backward();
    void motor2Brake();
    void motor2Coast();

   private:
    uint8_t _motor1Speed = 0;
    uint8_t _motor2Speed = 0;

   protected:
    static constexpr bool _MULTI_INSTANCE = false;
};
