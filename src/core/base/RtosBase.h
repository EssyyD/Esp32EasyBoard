#pragma once
#include <cstdint>

class RtosBase {
   public:
    const char* serviceName     = "rtos";
    bool        _rtosRunning    = false;
    uint32_t    rtosStackSize   = 4096;
    uint32_t    rtosTaskDelayMs = 10;
    const char* rtosTaskName    = "rtosTask";

    virtual void initRtos() = 0;
    void rtosRun();
    void rtosStop();
    void serviceDelay(uint32_t ms);

   protected:
    virtual void _delayHook(uint32_t ms) {}  // serviceDelay 中 vTaskDelay 之前调用（LVGL 服务覆写为 lv_tick_inc）
    virtual void _rtosBegin()          {}
    virtual void _rtosBeforeLoopOnce() {}
    virtual void _rtosLoop()           {}
    virtual void _rtosEnd()            {}

    static void _rtosTask(void* arg);
};
