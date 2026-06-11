#pragma once
#include "core/base/RtosBase.h"

#include <lvgl.h>
#include "device/spi/Lcd.Nv3007.spi.device.h"
#include "module/GifFileLvglPlayer.h"


class DisplayServiceRtos : public RtosBase {
   public:
    enum JointMode : uint8_t { LEFT_RIGHT, TOP_BOTTOM };

    const char* serviceName = "display";

    void initRtos() override {
        this->rtosTaskName   = this->serviceName;
        this->rtosStackSize  = 8192;
        this->rtosTaskDelayMs = 5;
    }

    LcdNv3007Device*   lcd1 = nullptr;
    LcdNv3007Device*   lcd2 = nullptr;

    lv_display_t* dispJoint = nullptr;
    GifFileLvglPlayer  gifPlayer;

   private:
    struct Pt { uint16_t x, y; };

    struct JointCtx {
        LcdNv3007Device*   lcd1;
        LcdNv3007Device*   lcd2;
        uint16_t*          buf1;
        uint16_t*          buf2;
        int32_t split;
        JointMode mode;
        lv_obj_t* scr;
        Pt pointLeftTop, pointRightTop, pointRightBottom, pointLeftBottom, pointCenter;
    };
    JointCtx _jointCtx;

    static constexpr int MAX_W = LcdNv3007Device::LONG_SIDE;
    static constexpr int LINES = 20;
    uint8_t _drawBuf1[MAX_W * LINES * 2];
    uint8_t _drawBuf2[MAX_W * LINES * 2];
    uint8_t _drawBufJ[MAX_W * 2 * LINES * 2];


   protected:
    void _delayHook(uint32_t ms) override;
    void _rtosBegin() override;
    void _rtosBeforeLoopOnce() override;
    void _rtosLoop()  override;
    void _rtosEnd()   override;

    void _lvglToJoint(JointMode mode);
    void _lvPortFsInit();
};



