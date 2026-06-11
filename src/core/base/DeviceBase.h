#pragma once
#include <Arduino.h>
#include <vector>
#include <cstring>
#include <driver/gpio.h>

struct PinDef { const char* owner; gpio_num_t pin; };

class EasyBoard;

class DeviceBase {
   public:
    const char* deviceName = "";
    bool booted = false;
    std::vector<const char*> bootServices;

    struct Pins {
        PinDef* begin() { return nullptr; }
        PinDef* end()   { return nullptr; }
    } PIN_GPIOS;

    virtual ~DeviceBase() = default;

    /// 子类必须重写，返回类型协变
    static DeviceBase* instance() { return nullptr; }

    bool boot(const char* serviceName);
    void unboot(const char* serviceName);

    virtual void setEasyBoard(EasyBoard* board) {}

   protected:
    /// 子类必须声明: static constexpr bool _MULTI_INSTANCE = true/false;
    /// true  = 该设备可有多实例, instance() 每次 new
    /// false = 单例, instance() 返回 static local
    static constexpr bool _MULTI_INSTANCE = true;
};
