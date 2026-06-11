#pragma once
#include "core/base/RtosBase.h"

#include <vector>
#include <deque>
#include <cstdint>

class VoiceEchoServiceRtos : public RtosBase {
   public:
    const char* serviceName = "voiceEcho";

    void initRtos() override {
        this->rtosTaskName   = this->serviceName;
        this->rtosStackSize  = 8192;
        this->rtosTaskDelayMs = 10;
    }

    struct Recorder {
        std::vector<int16_t> buffer;
        std::deque<int16_t>  preroll;
        bool recording      = false;
        int  silenceSamples = 0;
        int  loudStreak     = 0;
    };

    Recorder* rec = nullptr;

   private:
    void _rtosBegin() override;
    void _rtosLoop()  override;
    void _rtosEnd()   override;

    bool isLoud(const std::vector<int16_t>& chunk);
};
