# 核心框架(core/ 与 core/base/)

## RtosBase —— FreeRTOS task mixin

`src/core/base/RtosBase.{h,cpp}`。继承它的对象获得一个独立 FreeRTOS task。

### 属性(在 initRtos() 里覆盖默认值)

| 属性 | 默认 | 说明 |
|---|---|---|
| `rtosTaskName` | "rtosTask" | xTaskCreate 的 task 名(服务惯例 = serviceName) |
| `rtosStackSize` | 4096 | 栈大小(byte)。LVGL/音频服务用 8192 |
| `rtosTaskDelayMs` | 10 | 每轮 `_rtosLoop` 之间框架自动 vTaskDelay 的间隔 |
| `_rtosRunning` | false | 运行标志;rtosRun 置 true,rtosStop/任务退出置 false |

### 生命周期(必须记牢)

```
rtosRun()                        // 调用者线程(通常主线程)
  ├─ initRtos()                  // 子类设置属性
  ├─ 已在运行? → 打 [WARN] 直接返回
  ├─ _rtosBegin()                // ⚠ 主线程上下文,task 未创建,禁止 vTaskDelay
  ├─ _rtosRunning = true
  └─ xTaskCreate(_rtosTask, name, stack, this, 优先级1)

_rtosTask(self)                  // 新 task 上下文
  ├─ _rtosBeforeLoopOnce()       // 只跑一次,可以 delay/serviceDelay(阻塞后续 loop)
  ├─ while (_rtosRunning) { _rtosLoop(); vTaskDelay(rtosTaskDelayMs); }
  ├─ _rtosEnd()
  └─ vTaskDelete(nullptr)

rtosStop()                       // 注意:同步调 _rtosEnd() 并清标志
  ├─ _rtosEnd()                  // ⚠ 在调用者线程立即执行
  └─ _rtosRunning = false        // task 循环随后退出,会再跑一次 _rtosEnd
```

钩子分工:

- `_rtosBegin` — 设备 boot、资源分配(不能 delay)
- `_rtosBeforeLoopOnce` — 一次性演示/预热逻辑(可以 delay)
- `_rtosLoop` — 周期业务,框架自动 delay,子类不要自己 sleep 整个周期
- `_rtosEnd` — 设备 unboot、资源释放(注意 rtosStop 路径下会被执行两次,需幂等)

所有 task 优先级固定为 1,无 core 绑定。

## RtosBase::serviceDelay 与 _delayHook

所有 RTOS task 的延迟必须用 `serviceDelay(ms)` 而非裸 `vTaskDelay`:

```cpp
void serviceDelay(uint32_t ms);   // _delayHook(ms) + vTaskDelay
```

`_delayHook` 默认为空。LVGL 相关 task（如 DisplayServiceRtos）**必须覆写**为 `lv_tick_inc(ms)`,否则 LVGL 时基停走、动画/GIF 冻结。

## DeviceBase —— boot/unboot 引用计数

`src/core/base/DeviceBase.{h,cpp}`。核心字段:

```cpp
const char* deviceName;                  // 日志与冲突表使用
bool booted;                             // 是否已有任意服务持有
std::vector<const char*> bootServices;   // 持有者(serviceName)列表,strcmp 去重
```

语义:

- `boot(serviceName)` 返回 **true 仅当这是第一个持有者** —— 子类约定:`if (!Base::boot(name)) return false;` 之后才做硬件初始化。同名重复 boot 返回 false 且不重复计数。
- `unboot(serviceName)` 移除持有者;**列表清空才真正释放**。子类惯例:释放硬件前检查 `bootServices.empty()`(或在调用基类 unboot 前检查 `size()==1`)。
- 日志:`[BOOT] <device> <- '<service>'` / `[UNBOOT] <device> (remaining=N)`。

### 三个 CRTP 总线变体的 boot 行为差异

| 基类 | boot() 额外做什么 | 子类必须实现 |
|---|---|---|
| `DeviceGpioBase<D>` | 遍历 `PIN_GPIOS` 向 `bootEASYB->bootGPIO` 注册(需先 `setEasyBoard`) | — |
| `DeviceI2cBase<D,Bus>` | 调 `derived->initI2c()`,失败则回滚 unboot | `bool initI2c()`(典型:`this->i2c = EASYB.i2c0`) |
| `DeviceSpiBase<D,Bus>` | 注册引脚 + 每脚 `pinMode(OUTPUT)+HIGH`,再调 `derived->initSpi()`,失败回滚 | `bool initSpi()`(典型:`this->spi = EASYB.spiH`) |

注意:GPIO 变体经 `bootEASYB` 成员访问板子(需 `setEasyBoard()` 注入,EasyBoard::onboardDeviceGpio 已对 mic/spk/tb6612 注入);SPI 变体直接用全局 `EASYB` 宏。

## I2cSoftBase —— 软件 I2C

`src/core/base/I2cSoftBase.h`,纯头文件。GPIO 模拟 I2C 时序(开漏 + delayMicroseconds 半周期),
API 模拟 TwoWire:`beginTransmission/write/endTransmission/requestFrom/available/read`。

子类(放 `src/i2cSofts/`)只需覆盖 `initBus()` 设 `_sdaGpio/_sclGpio/_clock`,
以及 `onbusDevices()` 挂载该总线上的设备。示例:`I2cSoftExample`(SDA=17, SCL=18, 100kHz)。
实例挂在 `EasyBoard.i2cSofts` 结构体(当前注释着,无启用实例)。

## EasyBoard —— 板级单例

`src/core/EasyBoard.{h,cpp}`,宏 `EASYB == EasyBoard::instance()`。

### 成员速查

| 成员 | 类型 | 说明 |
|---|---|---|
| `spiH` / `spiF` | SPIClass* | HSPI / FSPI,initSpi() 按引脚是否 NC 决定创建 |
| `i2c0` | TwoWire*(= &Wire) | initI2c() 里 begin,400kHz |
| `wifi` | WifiEsp32S3Device | 见 connectivity-web-data.md |
| `devicesI2c.oledSsd13065` | OledSsd1306I2cDevice* | 0.91" OLED(当前引脚 NC) |
| `devicesSpi.lcdNv3007` / `lcdNv3007_2` | LcdNv3007Device* | 双 LCD;#2 hCasetOffset=14 |
| `devicesGpio.mic` / `spk` / `tb6612Wheels` | 设备* | I2S 麦克风/喇叭、电机驱动 |
| `servicesRtos.{voiceEcho,connectivity,motion,display}ServiceRtos` | 服务 | 值成员,直接 `.rtosRun()` |
| `chipWebServerRtos` | ChipWebServerRtos | 由 wifi._applyMode 自动启停 |
| `chipSqlite` | ChipSqlite | 需手动 boot |
| `neoPixel` | Adafruit_NeoPixel | 板载 WS2812(GPIO48),`setLed(r,g,b)` |

### GPIO 冲突注册表

```cpp
bool bootGPIO(gpio_num_t pin, const char* owner);  // NC 直接 true;重复占用打 CONFLICT 返回 false
void unbootGPIO(gpio_num_t pin);                   // swap-remove
void dumpPins();                                   // 打印 "=== GPIO Allocations ==="
```

容量 `MAX_PINS = 32`。这是**运行时自检**机制,不是硬保护——冲突只打日志。

## ChipSqlite / ChipWebServerRtos

详见 connectivity-web-data.md。要点:ChipSqlite 也用 serviceName 引用计数(`boot(EasyBoard*, serviceName)`,
首个 boot 才 `sqlite3_initialize + sqlite3_open`);ChipWebServerRtos 是 RtosBase 直接子类(非服务),
`_rtosLoop` 只做 `httpServer.handleClient()`。
