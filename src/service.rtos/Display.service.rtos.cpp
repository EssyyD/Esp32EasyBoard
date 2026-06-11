#include "Display.service.rtos.h"
#include "core/EasyBoard.h"
#include <LittleFS.h>

void DisplayServiceRtos::_delayHook(uint32_t ms) { lv_tick_inc(ms); }

void DisplayServiceRtos::_rtosBegin() {
    this->lcd1 = EASYB.devicesSpi.lcdNv3007;
    this->lcd2 = EASYB.devicesSpi.lcdNv3007_2;

    lv_init();
    lv_log_register_print_cb([](lv_log_level_t lv, const char* buf) {
        Serial.printf("[LVGL-%d] %s\n", (int)lv, buf);
    });
    this->_lvPortFsInit();

    if (this->lcd1->boot(this->serviceName)) {
        // this->lcd1->initLcdV();
        this->lcd1->initLcdH();
    }

    // if (this->lcd2->boot(this->serviceName)) {
    //     // this->lcd2->initLcdV();
    //     this->lcd2->initLcdH();
    // }

    this->_lvglToJoint(TOP_BOTTOM);
    if (this->dispJoint) {
        lv_obj_set_style_bg_color(this->_jointCtx.scr, lv_color_hex(0x000000), 0);
    }
}


void DisplayServiceRtos::_rtosBeforeLoopOnce() {
    int h[5];
    h[0] = this->gifPlayer.play(this->_jointCtx.scr, "S:/gifAnims/blackE/blackEWink.gif",
                                this->_jointCtx.pointCenter.x,      this->_jointCtx.pointCenter.y,      GifFileLvglPlayer::CENTER);
    // h[1] = this->gifPlayer.play(this->_jointCtx.scr, "S:/gifAnims/blackE/blackEHappy.gif",
    //                             this->_jointCtx.pointLeftTop.x,     this->_jointCtx.pointLeftTop.y,     GifFileLvglPlayer::LT);
    // h[2] = this->gifPlayer.play(this->_jointCtx.scr, "S:/gifAnims/blackE/blackEThink.gif",
    //                             this->_jointCtx.pointLeftBottom.x,  this->_jointCtx.pointLeftBottom.y,  GifFileLvglPlayer::LB);
    // h[3] = this->gifPlayer.play(this->_jointCtx.scr, "S:/gifAnims/blackE/blackEWink.gif",
    //                             this->_jointCtx.pointRightTop.x,    this->_jointCtx.pointRightTop.y,    GifFileLvglPlayer::RT);
    // h[4] = this->gifPlayer.play(this->_jointCtx.scr, "S:/gifAnims/blackE/blackESad.gif",
    //                             this->_jointCtx.pointRightBottom.x, this->_jointCtx.pointRightBottom.y, GifFileLvglPlayer::RB);


    // this->gifPlayer.clear(h[0]); this->serviceDelay(2000);
    // this->gifPlayer.clear(h[1]); this->serviceDelay(2000);
    // this->gifPlayer.clear(h[2]); this->serviceDelay(2000);
    // this->gifPlayer.clear(h[3]); this->serviceDelay(2000);
    // this->gifPlayer.clear(h[4]); 
}

void DisplayServiceRtos::_rtosLoop() {
    lv_tick_inc(this->rtosTaskDelayMs);
    this->gifPlayer.tickAll();
}

void DisplayServiceRtos::_rtosEnd() {
    this->gifPlayer.clearAll();
    if (this->dispJoint) { lv_display_delete(this->dispJoint); this->dispJoint = nullptr; }
    if (this->lcd1)  { this->lcd1->unboot(this->serviceName); }
    if (this->lcd2)  { this->lcd2->unboot(this->serviceName); }
}

void DisplayServiceRtos::_lvglToJoint(JointMode mode) {
    if (!this->lcd1->booted && !this->lcd2->booted) return;

    if (this->dispJoint) { lv_display_delete(this->dispJoint); this->dispJoint = nullptr; }

    int32_t jw, jh, split;
    if (this->lcd1->booted && this->lcd2->booted) {
        if (mode == LEFT_RIGHT) {
            jw = this->lcd1->width + this->lcd2->width;
            jh = (this->lcd1->height > this->lcd2->height) ? this->lcd1->height : this->lcd2->height;
            split = this->lcd1->width;
        } else if (mode == TOP_BOTTOM) {
            jw = (this->lcd1->width > this->lcd2->width) ? this->lcd1->width : this->lcd2->width;
            jh = this->lcd1->height + this->lcd2->height;
            split = this->lcd1->height;
        }
    } else if (this->lcd1->booted) {
        jw = this->lcd1->width;
        jh = this->lcd1->height;
        split = 0;
    } else if (this->lcd2->booted) {
        jw = this->lcd2->width;
        jh = this->lcd2->height;
        split = 0;
    }

    this->_jointCtx = {
        this->lcd1->booted ? this->lcd1 : nullptr,
        this->lcd2->booted ? this->lcd2 : nullptr,
        reinterpret_cast<uint16_t*>(this->_drawBuf1),
        reinterpret_cast<uint16_t*>(this->_drawBuf2),
        split,
        mode,
        nullptr,
        {0, 0},
        {uint16_t(jw - 1), 0},
        {uint16_t(jw - 1), uint16_t(jh - 1)},
        {0, uint16_t(jh - 1)},
        {uint16_t(jw / 2), uint16_t(jh / 2)},
    };

    this->dispJoint = lv_display_create(jw, jh);
    lv_display_set_user_data(this->dispJoint, &this->_jointCtx);
    lv_display_set_flush_cb(this->dispJoint, [](lv_display_t* disp, const lv_area_t* area, uint8_t* pxMap) {
        static int hit = 0;
        if (++hit <= 3) Serial.printf("[flushJ] #%d area=(%d,%d)-(%d,%d)\n", hit, area->x1, area->y1, area->x2, area->y2);
        auto* ctx = static_cast<JointCtx*>(lv_display_get_user_data(disp));
        int32_t w = area->x2 - area->x1 + 1;
        int32_t h = area->y2 - area->y1 + 1;
        auto* src = reinterpret_cast<uint16_t*>(pxMap);

        if (ctx->lcd1 && !ctx->lcd2) {
            ctx->lcd1->flushWrite(area->x1, area->y1, area->x2, area->y2, src, w * h);
        }else if (!ctx->lcd1 && ctx->lcd2) {
            ctx->lcd2->flushWrite(area->x1, area->y1, area->x2, area->y2, src, w * h);
        }else{
            if (ctx->mode == LEFT_RIGHT) {
                int32_t lx1 = area->x1;
                int32_t lx2 = area->x2;
                if (lx2 >= ctx->split) lx2 = ctx->split - 1;
                int32_t lw = (lx1 <= lx2) ? (lx2 - lx1 + 1) : 0;

                int32_t rx1 = area->x1;
                if (rx1 < ctx->split) rx1 = ctx->split;
                int32_t rx2 = area->x2;
                int32_t rw = (rx1 <= rx2) ? (rx2 - rx1 + 1) : 0;

                for (int32_t row = 0; row < h; row++) {
                    auto* srcRow = src + row * w;
                    if (lw > 0) {
                        auto* dst = ctx->buf1 + row * lw;
                        memcpy(dst, srcRow, lw * 2);
                    }
                    if (rw > 0) {
                        auto* dst = ctx->buf2 + row * rw;
                        memcpy(dst, srcRow + (rx1 - area->x1), rw * 2);
                    }
                }

                if (lw > 0 && ctx->lcd1) ctx->lcd1->flushWrite(lx1, area->y1, lx2, area->y2, ctx->buf1, lw * h);
                if (rw > 0 && ctx->lcd2) ctx->lcd2->flushWrite(rx1 - ctx->split, area->y1, rx2 - ctx->split, area->y2, ctx->buf2, rw * h);
            } else if (ctx->mode == TOP_BOTTOM) {
                int32_t topEnd = area->y2;
                if (topEnd >= ctx->split) topEnd = ctx->split - 1;
                if (area->y1 <= topEnd && ctx->lcd1) {
                    int32_t th = topEnd - area->y1 + 1;
                    ctx->lcd1->flushWrite(area->x1, area->y1, area->x2, topEnd, src, w * th);
                }

                int32_t botY1 = area->y1;
                if (botY1 < ctx->split) botY1 = ctx->split;
                if (botY1 <= area->y2 && ctx->lcd2) {
                    int32_t bh = area->y2 - botY1 + 1;
                    ctx->lcd2->flushWrite(area->x1, botY1 - ctx->split, area->x2, area->y2 - ctx->split,
                                        src + (botY1 - area->y1) * w, w * bh);
                }
            }
        }

        lv_display_flush_ready(disp);
    });
    lv_display_set_buffers(this->dispJoint, this->_drawBufJ, nullptr, sizeof(this->_drawBufJ), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_default(this->dispJoint);
    this->_jointCtx.scr = lv_screen_active();

    Serial.printf(
        "[Joint] %s %dx%d split=%d\n",
        mode == LEFT_RIGHT ? "LR" : "TB",
        jw,
        jh,
        split
    );
}


void DisplayServiceRtos::_lvPortFsInit() {
    static lv_fs_drv_t drv;
    lv_fs_drv_init(&drv);
    drv.letter     = 'S';
    drv.cache_size = 512;

    drv.open_cb = [](lv_fs_drv_t*, const char* path, lv_fs_mode_t mode) -> void* {
        File f = LittleFS.open(path, (mode == LV_FS_MODE_WR) ? "w" : "r");
        if (!f) return nullptr;
        return new File(std::move(f));
    };
    drv.close_cb = [](lv_fs_drv_t*, void* fp) { delete (File*)fp; return LV_FS_RES_OK; };
    drv.read_cb  = [](lv_fs_drv_t*, void* fp, void* buf, uint32_t len, uint32_t* br) -> lv_fs_res_t {
        size_t n = ((File*)fp)->read((uint8_t*)buf, len);
        if (br) *br = n;
        return LV_FS_RES_OK;
    };
    drv.seek_cb  = [](lv_fs_drv_t*, void* fp, uint32_t pos, lv_fs_whence_t w) -> lv_fs_res_t {
        SeekMode m = (w == LV_FS_SEEK_SET) ? SeekSet : (w == LV_FS_SEEK_CUR) ? SeekCur : SeekEnd;
        return ((File*)fp)->seek(pos, m) ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
    };
    drv.tell_cb  = [](lv_fs_drv_t*, void* fp, uint32_t* pos) -> lv_fs_res_t {
        *pos = ((File*)fp)->position();
        return LV_FS_RES_OK;
    };

    lv_fs_drv_register(&drv);
}
