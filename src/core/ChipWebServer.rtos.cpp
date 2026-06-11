#include "ChipWebServer.rtos.h"
#include "EasyBoard.h"

void ChipWebServerRtos::_rtosBegin() {
    this->httpServer.onNotFound([this]() { this->webServe(); this->apiRouter(); });
    this->httpServer.begin();
}

void ChipWebServerRtos::_rtosEnd() {
    this->httpServer.stop();
    Serial.println("[ChipWebServerRtos] stopped");
}

void ChipWebServerRtos::webServe() {
    if (this->httpServer.method() != HTTP_GET) return;

    String uri = this->httpServer.uri();
    if (uri == "/") { uri = String(this->chipWebRoot) + "/index.html"; }
    else { uri = String(this->chipWebRoot) + uri; }

    if (!LittleFS.exists(uri)) return;

    File f = LittleFS.open(uri, "r");
    String contentType = "text/plain";
    if (uri.endsWith(".html")) contentType = "text/html";
    else if (uri.endsWith(".js")) contentType = "application/javascript";
    else if (uri.endsWith(".css")) contentType = "text/css";
    this->httpServer.streamFile(f, contentType);
    f.close();
}

void ChipWebServerRtos::apiRouter() {
    if (this->httpServer.method() == HTTP_GET) return;

    String uri = this->httpServer.uri();
    if (!uri.startsWith("/chipWebApi")) {
        this->httpServer.send(404, "text/plain", "not found");
        return;
    }

    String requestBody = this->httpServer.arg("plain");
    Serial.printf("[ChipWebServerRtos] body: %s\n", requestBody.c_str());

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, requestBody);
    if (err) {
        JsonDocument empty;
        this->httpServer.send(
            400, "application/json",
            this->formatResponse("", "400", "请求参数无效", empty.as<JsonVariant>())
        );
        return;
    }

    String dataStr = doc["data"] | "";
    String signStr = doc["sign"] | "";

    JsonDocument fmtDoc;
    DeserializationError fmtErr = deserializeJson(fmtDoc, dataStr);
    if (fmtErr) {
        JsonDocument empty;
        this->httpServer.send(
            400, "application/json",
            this->formatResponse("", "400", "请求参数无效", empty.as<JsonVariant>())
        );
        return;
    }

    JsonObject apiFmtData = fmtDoc.as<JsonObject>();

    String apiTraceno;
    for (int i = 0; i < 16; i++) {
        char hex[] = "0123456789abcdef";
        apiTraceno += String(hex[esp_random() % 16]);
    }

    String apiCv = uri.substring(1);
    Serial.printf("[ChipWebServerRtos] traceno=%s cv=%s\n", apiTraceno.c_str(), apiCv.c_str());

    if (apiCv == "chipWebApi/status") {
        this->_actionStatus(apiTraceno, apiCv, apiFmtData, dataStr, signStr);
        return;
    }

    this->_actionNotFound(apiTraceno, apiCv, apiFmtData, dataStr, signStr);
}

void ChipWebServerRtos::_actionStatus(const String& apiTraceno, const String& apiCv, JsonObject apiFmtData, const String& data, const String& sign) {
    JsonDocument bizContentDoc;
    bizContentDoc["uptimeMillis"] = millis();
    bizContentDoc["freeHeap"] = ESP.getFreeHeap();
    String responseJson = this->formatResponse(apiTraceno, "100", "success", bizContentDoc.as<JsonVariant>());
    Serial.printf("[ChipWebServerRtos] -> 200 %s\n", responseJson.c_str());
    this->httpServer.send(200, "application/json", responseJson);
}

void ChipWebServerRtos::_actionNotFound(const String& apiTraceno, const String& apiCv, JsonObject apiFmtData, const String& data, const String& sign) {
    JsonDocument emptyDoc;
    this->httpServer.send(
        404, "application/json",
        this->formatResponse(apiTraceno, "104", "访问的API接口无效", emptyDoc.as<JsonVariant>())
    );
}

String ChipWebServerRtos::formatResponse(const String& apiTraceno, const String& code, const String& msg, JsonVariant bizContent) {
    JsonDocument responseDoc;
    responseDoc["version"]     = "1.0";
    responseDoc["apiTraceno"]  = apiTraceno;
    responseDoc["code"]        = code;
    responseDoc["msg"]         = msg;
    if (bizContent.isNull()) {
        responseDoc["bizContent"] = responseDoc["bizContent"].to<JsonObject>();
    } else {
        responseDoc["bizContent"] = bizContent;
    }
    String json;
    serializeJson(responseDoc, json);
    return json;
}
