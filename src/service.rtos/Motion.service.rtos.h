#pragma once
#include "core/base/RtosBase.h"

class MotionServiceRtos : public RtosBase {
   public:
    const char* serviceName = "motion";

    void initRtos() override;

   private:
    int _testPhase  = 0;
    int _printCycle = 0;

    void _rtosBegin() override;
    void _rtosLoop() override;
    void _rtosEnd() override;
};
