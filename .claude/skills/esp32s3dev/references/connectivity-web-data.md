# 联网 / Web API / 存储

涉及:`device/Wifi.esp32S3.device.{h,cpp}`、`service.rtos/Connectivity.service.rtos.{h,cpp}`、
`core/ChipWebServer.rtos.{h,cpp}`、`core/ChipSqlite.{h,cpp}`、`data/chipWeb/`、根仓库 `chipWeb/`。

## WifiEsp32S3Device(EASYB.wifi)

DeviceBase 直接子类(不挂总线)。AP 与 STA 独立开关,组合出 4 种模式:

```cpp
EASYB.wifi.boot(serviceName);    // 仅引用计数,无硬件动作
EASYB.wifi.apOn() / apOff();     // SoftAP
EASYB.wifi.staOn() / staOff();   // STA(连 _staSSID/_staPassword)
EASYB.wifi.off();                // 全关
```

- AP 默认:SSID `esp32-hotspot`、开放(密码空)、信道 1、最多 4 客户端
- 全部 set 方法带校验(密码 ≥8 位、信道 1~13 等),AP 运行中 set 会**立即重启 AP** 生效(`_reApIfActive`)
- AP 客户端跟踪:经 `esp_event_handler_register(WIFI_EVENT...)` 维护 `_apClients`(MAC 字符串表);
  `getAPClients/isAPClientOnline/kickAPClient`(kick 只移出列表,不真正断连)
- **关键联动**:`_applyMode()` 检测 WiFi 从 OFF↔ON 的边沿,自动
  `EASYB.chipWebServerRtos.rtosRun()/rtosStop()` —— Web 服务不需要手动启停

`ConnectivityServiceRtos`("connectivity")只是 `_rtosBegin` 里 `wifi.boot + apOn`,loop 空转(间隔 1h)。

## ChipWebServerRtos(端口 80)

RtosBase 子类("chipWebSvr",栈 6144,loop 50ms 调 `httpServer.handleClient()`)。
基于 Arduino `WebServer`,全部路由走 `onNotFound` → 先 `webServe()` 再 `apiRouter()`。

### 静态站点(webServe)

- 仅 GET;`/` → `/chipWeb/index.html`,其余 URI 前缀拼 `/chipWeb` 后查 LittleFS
- Content-Type 仅识别 .html/.js/.css,其余 text/plain

### JSON API 协议(apiRouter)

- 仅非 GET(前端用 POST);URI 必须以 `/chipWebApi` 开头,否则 404
- 请求体(双层 JSON,`data` 是字符串化的 JSON):

```json
{ "data": "{\"k\":\"v\"}", "sign": "..." }
```

- 响应统一由 `formatResponse(apiTraceno, code, msg, bizContent)` 生成:

```json
{ "version": "1.0", "apiTraceno": "16位随机hex", "code": "100", "msg": "success", "bizContent": { } }
```

- code 约定:`100`=成功,`400`=参数无效,`104`=API 不存在
- 现有接口:`POST /chipWebApi/status` → bizContent: `{uptimeMillis, freeHeap}`
- `sign` 字段目前透传未校验

### 新增 API 步骤

1. `ChipWebServer.rtos.h` 声明 `void _actionFoo(const String& apiTraceno, const String& apiCv, JsonObject apiFmtData, const String& data, const String& sign);`
2. `apiRouter()` 里加分支:

```cpp
if (apiCv == "chipWebApi/foo") { this->_actionFoo(apiTraceno, apiCv, apiFmtData, dataStr, signStr); return; }
```

3. 实现 `_actionFoo`:从 `apiFmtData`(已解析的 data 内层 JSON)取参,组 `JsonDocument bizContentDoc`,
   `httpServer.send(200, "application/json", formatResponse(apiTraceno, "100", "success", bizContentDoc.as<JsonVariant>()))`

## ChipSqlite(EASYB.chipSqlite)

Sqlite3Esp32 封装,DB 文件 `/chipSqlite/chipSqlite.db`(LittleFS)。**需手动 boot**,
与设备同款 serviceName 引用计数(但调用方式不同:`boot(EasyBoard*, serviceName)`,内部用 String 比较):

```cpp
EASYB.chipSqlite.boot(&EASYB, this->serviceName);     // 首个调用者触发 sqlite3_initialize+open
EASYB.chipSqlite.exec("CREATE TABLE IF NOT EXISTS t(k TEXT, v TEXT)");
EASYB.chipSqlite.queryRow("SELECT ...", [](int n, char** vals, char** cols){ ... });  // 取首行即停
EASYB.chipSqlite.queryAll("SELECT ...", cb);          // 遍历全部行
EASYB.chipSqlite.tableExists("t");
EASYB.chipSqlite.unboot(this->serviceName);           // 末个调用者 close + shutdown
```

注意:`queryRow` 的回调包装依赖 `{bool/int; std::function}` 结构体布局取偏移,改动时小心。
SQL 无参数化接口,拼接字符串时注意转义。

## chipWeb 前端(根仓库 esp32/chipWeb/)

- Vue 3 + Vite 6,`vite.config.js` 已设 `base: './'`(相对路径,适配 LittleFS 子路径)
- 开发:`npm run dev`;构建:`npm run build` → `dist/`
- **部署**:把 `dist/` 内容拷到 `esp32S3Dev/data/chipWeb/`(index.html + assets/),再 `pio run -t uploadfs`
- 访问:连上 ESP32 的 AP(esp32-hotspot)后浏览器开 `http://192.168.4.1/`

## data/ 与 LittleFS

```
data/                 ← pio run -t uploadfs 整体打包为 LittleFS 镜像
├── chipWeb/          # 前端(上面)
└── gifAnims/blackE/  # GIF 表情(display-lvgl.md)
```

运行期挂载在 `EasyBoard::init()`:`LittleFS.begin(false)`(不自动格式化)。
分区表 `default_16MB.csv`(LittleFS 分区约 ~6.9MB,放 GIF 时注意总量)。
