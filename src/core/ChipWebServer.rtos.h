#pragma once
#include "core/base/RtosBase.h"
#include <WebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <FS.h>

class ChipWebServerRtos : public RtosBase {
   public:
    const char* serviceName = "chipWebSvr";
    const char* chipWebRoot = "/chipWeb";
    WebServer httpServer{80};

    void initRtos() override {
        this->rtosTaskName    = "chipWebSvr";
        this->rtosStackSize   = 6144;
        this->rtosTaskDelayMs = 50;
    }

    String formatResponse(const String& apiTraceno, const String& code, const String& msg, JsonVariant bizContent);

   private:
    void _rtosBegin() override;
    void _rtosLoop()  override { this->httpServer.handleClient(); }
    void _rtosEnd()   override;

    void webServe();
    void apiRouter();

    void _actionStatus(const String& apiTraceno, const String& apiCv, JsonObject apiFmtData, const String& data, const String& sign);
    void _actionNotFound(const String& apiTraceno, const String& apiCv, JsonObject apiFmtData, const String& data, const String& sign);
};
