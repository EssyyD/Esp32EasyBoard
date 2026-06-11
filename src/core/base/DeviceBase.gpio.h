#pragma once
#include "DeviceBase.h"

class EasyBoard;

template<typename Derived>
class DeviceGpioBase : public DeviceBase {
   public:
    EasyBoard* bootEASYB = nullptr;

    void setEasyBoard(EasyBoard* board) override { this->bootEASYB = board; }

    bool boot(const char* serviceName);
    void unboot(const char* serviceName);
};
