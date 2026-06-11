#pragma once
#include <lvgl.h>

extern "C" {
#include "libs/gif/AnimatedGIF.h"
}

class GifFileLvglPlayer {
public:
    enum Anchor : uint8_t { LT, RT, CENTER, LB, RB, LC, RC, TC, BC };

    static constexpr int MAX_GIFS = 8;
    static constexpr int INVALID   = -1;

    GifFileLvglPlayer() { for (int i = 0; i < MAX_GIFS; i++) this->_gifs[i] = nullptr; }
    ~GifFileLvglPlayer() { this->clearAll(); }

    int play(lv_obj_t* parent, const char* path,
             int16_t x = -1, int16_t y = -1,
             Anchor anchor = CENTER) {
        for (int i = 0; i < MAX_GIFS; i++) {
            if (!this->_gifs[i]) {
                auto* gif = new Gif();
                if (!this->_gifOpen(gif, path)) { delete gif; return INVALID; }
                this->_gifAttach(gif, parent, x, y, anchor);
                this->_gifs[i] = gif;
                this->tickAll();
                return i;
            }
        }
        return INVALID;
    }

    void setPos(int handle, int16_t x, int16_t y, Anchor anchor = LT) {
        if (handle < 0 || handle >= MAX_GIFS || !this->_gifs[handle]) return;
        lv_obj_set_pos(this->_gifs[handle]->img,
                       _anchorX(x, this->_gifs[handle]->width, anchor),
                       _anchorY(y, this->_gifs[handle]->height, anchor));
        this->tickAll();
    }

    void clear(int handle) {
        if (handle < 0 || handle >= MAX_GIFS || !this->_gifs[handle]) return;
        this->_gifClose(this->_gifs[handle]);
        delete this->_gifs[handle];
        this->_gifs[handle] = nullptr;
        this->tickAll();
    }
    void clearAll() {
        for (int i = 0; i < MAX_GIFS; i++) {
            if (this->_gifs[i]) {
                this->_gifClose(this->_gifs[i]);
                delete this->_gifs[i];
                this->_gifs[i] = nullptr;
            }
        }
        this->tickAll();
    }

    void tickAll() {
        uint32_t now = millis();
        if (now - this->_lastTickMs < 10) return;
        this->_lastTickMs = now;
        for (int i = 0; i < MAX_GIFS; i++) {
            if (this->_gifs[i]) this->_gifTick(this->_gifs[i]);
        }
        lv_timer_handler();
    }

    int width(int handle)  const { return (handle >= 0 && handle < MAX_GIFS && this->_gifs[handle]) ? this->_gifs[handle]->width  : 0; }
    int height(int handle) const { return (handle >= 0 && handle < MAX_GIFS && this->_gifs[handle]) ? this->_gifs[handle]->height : 0; }

private:
    struct Gif {
        GIFIMAGE        gif = {};
        uint8_t*        canvas = nullptr;
        int             canvasSize = 0;
        int             width = 0, height = 0;
        lv_obj_t*       img = nullptr;
        lv_image_dsc_t  imgDsc = {};
        uint32_t        nextFrameMs = 0;
        int             prevX = 0, prevY = 0, prevW = 0, prevH = 0;
        uint8_t         prevDisposal = 0xFF;
        uint8_t         palBg[3] = {};
    };
    Gif* _gifs[MAX_GIFS];
    uint32_t _lastTickMs = 0;

    bool _gifOpen(Gif* gif, const char* path) {
        GIF_begin(&gif->gif, GIF_PALETTE_RGB565_LE);
        if (!GIF_openFile(&gif->gif, path, _drawCb)) {
            Serial.printf("[GifPlayer] open FAIL: %s err=%d\n", path, gif->gif.iError);
            return false;
        }
        gif->width  = GIF_getCanvasWidth(&gif->gif);
        gif->height = GIF_getCanvasHeight(&gif->gif);
        gif->canvasSize = gif->width * gif->height * 2;
        gif->canvas = (uint8_t*)malloc(gif->canvasSize);
        if (!gif->canvas) {
            Serial.printf("[GifPlayer] malloc(%d) FAIL\n", gif->canvasSize);
            GIF_close(&gif->gif);
            return false;
        }
        memset(gif->canvas, 0, gif->canvasSize);
        int delayMs;
        this->_gifDecode(gif, &delayMs);
        gif->nextFrameMs = millis() + delayMs;
        Serial.printf("[GifPlayer] open OK %dx%d\n", gif->width, gif->height);
        return true;
    }

    void _gifAttach(Gif* gif, lv_obj_t* parent, int16_t x, int16_t y, Anchor anchor) {
        gif->imgDsc.header.magic  = LV_IMAGE_HEADER_MAGIC;
        gif->imgDsc.header.cf     = LV_COLOR_FORMAT_RGB565;
        gif->imgDsc.header.flags  = LV_IMAGE_FLAGS_MODIFIABLE;
        gif->imgDsc.header.w      = gif->width;
        gif->imgDsc.header.h      = gif->height;
        gif->imgDsc.header.stride = gif->width * 2;
        gif->imgDsc.data          = gif->canvas;
        gif->imgDsc.data_size     = gif->canvasSize;
        gif->img = lv_image_create(parent);
        lv_image_set_src(gif->img, &gif->imgDsc);
        if (x == -1 && y == -1)
            lv_obj_center(gif->img);
        else
            lv_obj_set_pos(gif->img, _anchorX(x, gif->width, anchor), _anchorY(y, gif->height, anchor));
    }

    void _gifClose(Gif* gif) {
        if (gif->img)    { lv_obj_del(gif->img); gif->img = nullptr; }
        if (gif->canvas) { free(gif->canvas); gif->canvas = nullptr; }
        GIF_close(&gif->gif);
    }

    void _gifTick(Gif* gif) {
        if (!gif->canvas || !gif->img) return;
        uint32_t now = millis();
        if (now < gif->nextFrameMs) return;
        int delayMs;
        if (!this->_gifDecode(gif, &delayMs)) {
            GIF_reset(&gif->gif);
            gif->prevDisposal = 0xFF;
            this->_gifDecode(gif, &delayMs);
        }
        gif->nextFrameMs = now + delayMs;
        lv_image_set_src(gif->img, &gif->imgDsc);
        lv_obj_invalidate(gif->img);
    }

    bool _gifDecode(Gif* gif, int* delayMs) {
        if (gif->prevDisposal == 2) {
            uint16_t bg565 = ((gif->palBg[0] >> 3) << 11) | ((gif->palBg[1] >> 2) << 5) | (gif->palBg[2] >> 3);
            int x1 = gif->prevX, y1 = gif->prevY;
            int x2 = gif->prevX + gif->prevW - 1, y2 = gif->prevY + gif->prevH - 1;
            if (x2 >= gif->width)  x2 = gif->width  - 1;
            if (y2 >= gif->height) y2 = gif->height - 1;
            for (int y = y1; y <= y2; y++) {
                auto* row = (uint16_t*)(gif->canvas + y * gif->width * 2);
                for (int x = x1; x <= x2; x++) row[x] = bg565;
            }
        } else if (gif->prevDisposal == 0xFF || gif->prevDisposal > 2) {
            memset(gif->canvas, 0, gif->canvasSize);
        }
        gif->gif.pUser = gif;
        return GIF_playFrame(&gif->gif, delayMs, gif) > 0;
    }

    static void _drawCb(GIFDRAW* pDraw) {
        auto* gif = (Gif*)pDraw->pUser;
        uint16_t* palette    = pDraw->pPalette;
        uint8_t*  palette24  = pDraw->pPalette24;
        int       lineY      = pDraw->iY + pDraw->y;
        uint16_t* dst        = (uint16_t*)(gif->canvas + lineY * gif->width * 2) + pDraw->iX;
        uint8_t*  src        = pDraw->pPixels;

        if (pDraw->y == 0) {
            gif->prevX = pDraw->iX;
            gif->prevY = pDraw->iY;
            gif->prevW = pDraw->iWidth;
            gif->prevH = pDraw->iHeight;
            gif->prevDisposal = pDraw->ucDisposalMethod;
            uint8_t bg = pDraw->ucBackground;
            if (palette24) {
                gif->palBg[0] = palette24[bg * 3 + 0];
                gif->palBg[1] = palette24[bg * 3 + 1];
                gif->palBg[2] = palette24[bg * 3 + 2];
            }
        }

        if (pDraw->ucHasTransparency) {
            uint8_t transparent = pDraw->ucTransparent;
            for (int x = 0; x < pDraw->iWidth; x++)
                if (src[x] != transparent) dst[x] = palette[src[x]];
        } else {
            for (int x = 0; x < pDraw->iWidth; x++)
                dst[x] = palette[src[x]];
        }
    }

    static int16_t _anchorX(int16_t x, int16_t w, Anchor a) {
        switch (a) {
            case RT: case RB: case RC: return x - w;
            case CENTER: case TC: case BC: return x - w / 2;
            default: return x;
        }
    }
    static int16_t _anchorY(int16_t y, int16_t h, Anchor a) {
        switch (a) {
            case LB: case RB: case BC: return y - h;
            case CENTER: case LC: case RC: return y - h / 2;
            default: return y;
        }
    }
};
