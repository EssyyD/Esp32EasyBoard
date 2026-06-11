# 显示子系统(NV3007 + LVGL 9 + GIF)

涉及文件:
`device/spi/Lcd.Nv3007.spi.device.{h,cpp}`、`service.rtos/Display/Display.service.rtos.{h,cpp}`、
`service.rtos/Display/GifFileLvglPlayer.h`、`include/lv_conf.h`、`data/gifAnims/`。

## LcdNv3007Device(1.78" 条形 LCD,142×428,4 线 SPI)

- 多实例(`_MULTI_INSTANCE = true`):`lcdNv3007`(主)与 `lcdNv3007_2`(副,当前引脚 NC)
- SPI:`spiH` @ 50MHz、MSBFIRST、**SPI_MODE3**;手动控制 CS/DC(原生 SPIClass,不用 Arduino_GFX,库已移除)
- `_tftInit()`:厂商初始化序列(0xFF 0xA5 进私有寄存器页 → gamma/电源/扫描配置 → 0x11 Sleep Out → 0x29 Display On)

### 坐标与偏移(改显示方向时必须理解)

| 模式 | 调用 | width×height | CASET 偏移 | RASET 偏移 |
|---|---|---|---|---|
| 竖屏 'V' | `initLcdV()` | 142×428 | 12(`ROTATION0_OFFSET`) | 0 |
| 横屏 'H' | `initLcdH()`(MADCTL 0x36=0xB0) | 428×142 | `hCasetOffset`(面板1=0,面板2=14) | 12 |

面板存在 12/14 像素的驱动 IC 偏移;`hCasetOffset` 由 `instance()` 第 5 参传入
(EasyBoard 里 `lcdNv3007_2` 传 `ROTATION1_OFFSET`=14)。新面板若画面错位,调这两个常量。

`initLcdV/H` 同时填好 `pointLeftTop/RightTop/RightBottom/LeftBottom/Center` 五个定位点和 `coordMode`。

### flushWrite —— 唯一的像素出口

```cpp
void flushWrite(x1, y1, x2, y2, uint16_t* pxMap, size_t len);
// 0x2A CASET(+caOffset) → 0x2B RASET(+raOffset) → 0x2C 写 RGB565 大端像素流
```

LVGL flush 回调最终都汇到这里。像素格式 RGB565,逐字节 `transfer(hi); transfer(lo)`。

## DisplayServiceRtos(拼接显示服务)

"display",栈 8192,loop 5ms。核心思想:**两块物理屏拼成一个 LVGL 逻辑大屏**。

### _rtosBegin 流程

1. `lv_init()` + 注册 LVGL 日志回调(`[LVGL-n]` 打到 Serial)
2. `_lvPortFsInit()`:注册 LVGL 文件系统驱动,盘符 **'S' → LittleFS**(open/read/seek/tell/close 直通 File)
3. boot lcd1(当前 lcd2 注释停用)→ `initLcdH()` 横屏
4. `_lvglToJoint(TOP_BOTTOM)` 创建拼接 display,屏幕背景设黑

### _lvglToJoint(JointMode)

- `LEFT_RIGHT`:逻辑宽 = w1+w2,split = lcd1->width(x 方向切)
- `TOP_BOTTOM`:逻辑高 = h1+h2,split = lcd1->height(y 方向切)
- 只有一块屏 booted 时退化为单屏直通
- `lv_display_create(jw, jh)` + 自定义 `flush_cb`:按 split 把刷新区裁成两half,分别
  `lcd1->flushWrite(...)` / `lcd2->flushWrite(坐标减 split, ...)`,LEFT_RIGHT 模式需逐行 memcpy 到 `_drawBuf1/2` 重排
- 渲染缓冲:`_drawBufJ`(428×2 宽 × 20 行 × 2 字节),`LV_DISPLAY_RENDER_MODE_PARTIAL`
- 拼接后的定位点存 `_jointCtx.pointLeftTop/.../pointCenter`,GIF 摆位用它们

### LVGL 时基(重要)

- `_rtosLoop()`:`lv_tick_inc(rtosTaskDelayMs)` + `gifPlayer.tickAll()`
- `lv_timer_handler()` 在 `GifFileLvglPlayer::tickAll()` 内部调用(≥10ms 节流)
- 服务里长等待用 `serviceDelay()`(会补 lv_tick_inc),否则动画冻结

## GifFileLvglPlayer(多路 GIF 播放器)

纯头文件类。解码器用 **LVGL 自带的 AnimatedGIF C 库**(`lvgl/src/libs/gif/AnimatedGIF.h`,
经 `extern "C"` 包含;lv_conf 里 `LV_USE_GIF 1` 保证其参与编译)。不用 lv_gif 控件,
自己 malloc RGB565 canvas + `lv_image` 显示,帧推进/disposal 自己管理。

### API

```cpp
int  play(lv_obj_t* parent, const char* path, int16_t x=-1, int16_t y=-1, Anchor anchor=CENTER);
     // path 形如 "S:/gifAnims/blackE/blackEWink.gif"(S: = LittleFS)
     // 返回 handle(0~7),失败 INVALID(-1);x=y=-1 → lv_obj_center
void setPos(int handle, int16_t x, int16_t y, Anchor anchor=LT);
void clear(int handle);   void clearAll();
void tickAll();           // 帧推进 + lv_timer_handler,Display 服务每 loop 调
int  width/height(int handle);
```

- 最多 `MAX_GIFS = 8` 路同播
- `Anchor`:LT/RT/CENTER/LB/RB/LC/RC/TC/BC —— (x,y) 是锚点对齐坐标(如 RB = 右下角贴 (x,y))
- 循环播放:`GIF_playFrame` 返回 0 时 `GIF_reset` 重头
- disposal 2(恢复背景色)与 disposal ≥3/首帧(清黑)已正确处理;透明像素跳写

### 内存注意

canvas = W×H×2 字节,走 `malloc`(经 PSRAM 分配大块)。428×284 整屏 GIF ≈ 243KB,8 路上限注意 PSRAM 余量。

## lv_conf.h 关键配置(include/lv_conf.h,v9.5 配置文件)

| 配置 | 值 | 说明 |
|---|---|---|
| `LV_COLOR_DEPTH` | 16 | RGB565 |
| `LV_MEM_SIZE` | 32KB | LVGL 内部堆(控件多了需调大) |
| `LV_DEF_REFR_PERIOD` | 33ms | |
| `LV_USE_OS` | LV_OS_NONE | 单线程使用,勿跨 task 调 LVGL API |
| `LV_USE_GIF` | 1 | 启用 AnimatedGIF 解码库 |
| `LV_USE_FS_*` | 全 0 | 内置 FS 驱动全关,用 Display 服务自注册的 'S' 驱动 |

platformio.ini 用 `-DLV_CONF_INCLUDE_SIMPLE -Iinclude` 让 lvgl(^9.3.0)找到此文件。

## GIF 资源管线

1. 素材 GIF 放 `tools/gif2cpp/input/`(根仓库 `esp32/tools/`),`gif2cpp.py` 可缩放(默认高 142)/转 RGB565
2. 成品 `.gif` 放 `esp32S3Dev/data/gifAnims/blackE/`(现有 20 个表情:Angry/Happy/Wink/...)
3. `pio run -t uploadfs` 烧入 LittleFS
4. 代码里 `gifPlayer.play(scr, "S:/gifAnims/blackE/blackEHappy.gif", cx, cy, GifFileLvglPlayer::CENTER)`
