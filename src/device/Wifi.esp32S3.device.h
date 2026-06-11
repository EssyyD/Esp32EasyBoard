#pragma once
#include "core/base/DeviceBase.h"
#include <WiFi.h>
#include <esp_event.h>
#include <vector>

class WifiEsp32S3Device : public DeviceBase {
   public:
    const char* deviceName = "Wifi.ESP32S3";

    // ---- AP getter ----
    const char* getApSSID()          const { return this->_apSSID; }
    const char* getApPassword()      const { return this->_apPassword; }
    int         getApChannel()       const { return this->_apChannel; }
    int         getApMaxConnections() const { return this->_apMaxConnections; }
    bool        getApHidden()        const { return this->_apHidden; }
    IPAddress   getApIP()            const { return this->_apIP; }
    IPAddress   getApGateway()       const { return this->_apGateway; }
    IPAddress   getApSubnet()        const { return this->_apSubnet; }

    // ---- AP setter (运行中调用会立即重配，返回 false 表示参数无效) ----
    bool setApSSID(const char* ssid)          { if (!ssid || !*ssid)  return false; this->_apSSID = ssid;          this->_reApIfActive(); return true; }
    bool setApPassword(const char* pwd)       { if (!pwd  || strlen(pwd) < 8) return false; this->_apPassword = pwd; this->_reApIfActive(); return true; }
    bool setApChannel(int ch)                 { if (ch < 1 || ch > 13)       return false; this->_apChannel = ch;    this->_reApIfActive(); return true; }
    bool setApMaxConnections(int n)           { if (n < 1 || n > 10)         return false; this->_apMaxConnections = n; this->_reApIfActive(); return true; }
    bool setApHidden(bool hidden)             { this->_apHidden = hidden;     this->_reApIfActive(); return true; }

    // ---- STA getter ----
    const char* getStaSSID()     const { return this->_staSSID; }
    const char* getStaPassword() const { return this->_staPassword; }

    // ---- STA setter ----
    bool setStaSSID(const char* ssid)     { if (!ssid || !*ssid) return false; this->_staSSID = ssid;     return true; }
    bool setStaPassword(const char* pwd)  { if (!pwd  || strlen(pwd) < 8) return false; this->_staPassword = pwd; return true; }

    // ---- AP 客户端 ----
    std::vector<String> getAPClients() const { return this->_apClients; }
    bool isAPClientOnline(const char* mac);
    void kickAPClient(const char* mac);

    bool boot(const char* serviceName);
    void unboot(const char* serviceName);

    void staOn();
    void staOff();
    void apOn();
    void apOff();
    void off();

    bool isSTA() const { return this->_staActive; }
    bool isAP()  const { return this->_apActive; }

    virtual void onWiFiEventAPStaConnected(const uint8_t* mac);
    virtual void onWiFiEventAPStaDisconnected(const uint8_t* mac);

   private:
    bool _staActive = false;
    bool _apActive  = false;

    const char* _apSSID           = "esp32-hotspot";
    const char* _apPassword       = "";
    int         _apChannel        = 1;
    int         _apMaxConnections = 4;
    bool        _apHidden         = false;
    IPAddress   _apIP;
    IPAddress   _apGateway;
    IPAddress   _apSubnet;

    const char* _staSSID     = "";
    const char* _staPassword = "";

    std::vector<String> _apClients;

    void _applyMode();
    void _reApIfActive();
    void _softAP();
    int  _findClient(const char* mac) const;

    friend void wiFiEventDispatch(void* arg, esp_event_base_t base, int32_t id, void* data);
};
