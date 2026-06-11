#pragma once
#include "core/base/RtosBase.h"

class ConnectivityServiceRtos : public RtosBase {
   public:
    const char* serviceName = "connectivity";

    void initRtos() override {
        this->rtosTaskName    = this->serviceName;
        this->rtosStackSize   = 4096;
        this->rtosTaskDelayMs = 3600000;
    }

   private:
    void _rtosBegin() override;
    void _rtosLoop()  override;
    void _rtosEnd()   override;
};
