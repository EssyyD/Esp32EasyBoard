# 服务层(src/service.rtos/)

服务 = `ServiceBaseRtos` 子类 = 一个 FreeRTOS task。生命周期钩子语义见 core-framework.md。
启用方式:在 `main.cpp` 的 `setup()` 里 `EASYB.servicesRtos.<xxx>.rtosRun();`(注释 = 停用)。

## 服务清单

| 服务 | serviceName | 栈 | loop 间隔 | 用到的设备 | 功能 |
|---|---|---|---|---|---|
| `MotionServiceRtos` | "motion" | 4096 | 20ms | tb6612Wheels(+motor1) | 测试:50 拍前进/50 拍后退,每 10 拍打印编码器 |
| `VoiceEchoServiceRtos` | "voiceEcho" | 8192 | 10ms | mic、spk、oledSsd13065 | VAD 检测→录音→静音/超时结束→喇叭回放 |
| `ConnectivityServiceRtos` | "connectivity" | 4096 | 3600000ms | wifi | 开 WiFi AP(loop 空转) |
| `DisplayServiceRtos` | "display" | 8192 | 5ms | lcdNv3007(_2) | LVGL 拼接显示 + GIF 表情(见 display-lvgl.md) |

另有非服务的 RtosBase 任务:`ChipWebServerRtos`("chipWebSvr",6144,50ms),由 WiFi 状态自动启停。

## 各服务要点

### Motion

`_rtosBegin` boot TB6612 并 `motor1->setSpeed(100)`;`_rtosEnd` brake + unboot。
典型的最小服务样板,新写服务可参照它。

### VoiceEcho(参数都在 config/deviceSets.voiceEcho.config.h)

| 宏 | 值 | 含义 |
|---|---|---|
| `DEVICE_SET_VOICE_ECHO_MIC_SAMPLE_RATE` / `SPK_SAMPLE_RATE` | 16000 | 采样率 |
| `DEVICE_SET_VOICE_ECHO_SPK_VOLUME` | 70 | 回放音量 % |
| `DEVICE_SET_VOICE_ECHO_VAD_THRESHOLD` | 0.10f | 响度阈值(归一化幅值) |
| `DEVICE_SET_VOICE_ECHO_VAD_DEBOUNCE` | 5 | 连续 N 个响 chunk 才触发录音 |
| `DEVICE_SET_VOICE_ECHO_PREROLL_MS` | 400 | 触发前保留的预滚音频 |
| `DEVICE_SET_VOICE_ECHO_SILENCE_SEC` | 1 | 静音多久判定结束 |
| `DEVICE_SET_VOICE_ECHO_MAX_RECORD_SEC` | 8 | 最长录音 |

流程:`_rtosLoop` 每轮 `mic->read(chunk, 480)`;未录音时维护 preroll 环形队列 + 响度去抖;
录音中累积 buffer,静音/超时结束后丢弃 <0.3s 的片段,否则按 960 样本分块 `spk->write` 回放(尾部补 4 块静音)。
设备 boot 失败 → OLED 显示 FAIL + LED 红 + 死循环。LED 状态:绿=监听,黄=录音,青=播放。

### Connectivity

`_rtosBegin`:`wifi.boot` + `wifi.apOn()`(AP 默认 SSID "esp32-hotspot",开放网络)。
AP 一开,`wifi._applyMode()` 会自动 `chipWebServerRtos.rtosRun()`(见 connectivity-web-data.md)。

### Display → 见 display-lvgl.md

## 新增服务步骤(以 Foo 为例)

1. `src/service.rtos/Foo.service.rtos.h`:

```cpp
#pragma once
#include "core/base/RtosBase.h"

class FooServiceRtos : public RtosBase {
   public:
    const char* serviceName = "foo";

    void initRtos() override {
        this->rtosTaskName    = this->serviceName;
        this->rtosStackSize   = 4096;      // 用 LVGL/音频/JSON 的话给 8192
        this->rtosTaskDelayMs = 20;
    }

   private:
    void _rtosBegin() override;   // 主线程:boot 设备,禁止 delay
    void _rtosLoop()  override;   // task 循环:业务逻辑
    void _rtosEnd()   override;   // 清理:unboot 设备(注意幂等)
};
```

2. `src/service.rtos/Foo.service.rtos.cpp`:

```cpp
#include "Foo.service.rtos.h"
#include "core/EasyBoard.h"

void FooServiceRtos::_rtosBegin() {
    EASYB.devicesSpi.fooBar->boot(this->serviceName);
}

void FooServiceRtos::_rtosLoop() {
    // 周期业务;框架自动 vTaskDelay(rtosTaskDelayMs)
}

void FooServiceRtos::_rtosEnd() {
    EASYB.devicesSpi.fooBar->unboot(this->serviceName);
}
```

3. 注册:`core/EasyBoard.h` include 头文件 + `servicesRtos` struct 加值成员:

```cpp
struct {
    FooServiceRtos fooServiceRtos;
    // …已有服务…
} servicesRtos;
```

4. 启用:`main.cpp` 里 `EASYB.servicesRtos.fooServiceRtos.rtosRun();`

### 注意事项

- 多服务可共享同一设备:boot/unboot 按 serviceName 引用计数,谁先 boot 谁触发硬件初始化。共享时确认参数(如采样率)在首个 boot 前已设置一致。
- 需要一次性带延时的开机演示 → 放 `_rtosBeforeLoopOnce()`,用 `serviceDelay()`。
- 服务间无消息机制,当前靠直接访问 `EASYB.servicesRtos.xxx` 公共成员/设备状态。
- `EasyBoard.h` 中存在循环包含风险(服务头 include EasyBoard.h 会环);现有服务都只在 .cpp 里 include `core/EasyBoard.h`,头文件只 include 基类——保持这个习惯。
