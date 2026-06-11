#include "Wifi.esp32S3.device.h"
#include "core/EasyBoard.h"

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

static String _macToStr(const uint8_t* mac) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", MAC2STR(mac));
    return String(buf);
}

void wiFiEventDispatch(void* arg, esp_event_base_t base, int32_t id, void* data) {
    auto* self = static_cast<WifiEsp32S3Device*>(arg);
    if (base == WIFI_EVENT) {
        if (id == WIFI_EVENT_AP_STACONNECTED) {
            auto* event = (wifi_event_ap_staconnected_t*)data;
            self->onWiFiEventAPStaConnected(event->mac);
        }
        else if (id == WIFI_EVENT_AP_STADISCONNECTED) {
            auto* event = (wifi_event_ap_stadisconnected_t*)data;
            self->onWiFiEventAPStaDisconnected(event->mac);
        }
    }
}

bool WifiEsp32S3Device::boot(const char* serviceName) {
    if (!DeviceBase::boot(serviceName)) return false;
    return true;
}

void WifiEsp32S3Device::unboot(const char* serviceName) {
    if (this->bootServices.size() == 1) {
        this->off();
    }
    DeviceBase::unboot(serviceName);
}

void WifiEsp32S3Device::staOn() {
    if (this->_staActive) return;
    this->_staActive = true;
    this->_applyMode();
    WiFi.begin(this->_staSSID, this->_staPassword);
}

void WifiEsp32S3Device::staOff() {
    if (!this->_staActive) return;
    WiFi.disconnect(true);
    this->_staActive = false;
    this->_applyMode();
}

void WifiEsp32S3Device::apOn() {
    if (this->_apActive) return;
    this->_apActive = true;
    this->_applyMode();
    this->_softAP();
}

void WifiEsp32S3Device::apOff() {
    if (!this->_apActive) return;
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wiFiEventDispatch);
    WiFi.softAPdisconnect(true);
    this->_apActive = false;
    this->_apClients.clear();
    this->_applyMode();
}

void WifiEsp32S3Device::off() {
    this->_staActive = false;
    this->_apActive  = false;
    WiFi.mode(WIFI_OFF);
    this->_applyMode();
}

bool WifiEsp32S3Device::isAPClientOnline(const char* mac) {
    return this->_findClient(mac) >= 0;
}

void WifiEsp32S3Device::kickAPClient(const char* mac) {
    int idx = this->_findClient(mac);
    if (idx < 0) return;
    this->_apClients.erase(this->_apClients.begin() + idx);
}

void WifiEsp32S3Device::onWiFiEventAPStaConnected(const uint8_t* mac) {
    this->_apClients.push_back(_macToStr(mac));
}

void WifiEsp32S3Device::onWiFiEventAPStaDisconnected(const uint8_t* mac) {
    int idx = this->_findClient(_macToStr(mac).c_str());
    if (idx >= 0) this->_apClients.erase(this->_apClients.begin() + idx);
}

void WifiEsp32S3Device::_softAP() {
    Serial.printf("[Wifi] _softAP: SSID='%s', PWD='%s', ch=%d, hidden=%d, max=%d\n",
                  this->_apSSID, this->_apPassword, this->_apChannel, this->_apHidden, this->_apMaxConnections);
    WiFi.softAP(
        this->_apSSID,
        this->_apPassword,
        this->_apChannel,
        this->_apHidden,
        this->_apMaxConnections
    );
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wiFiEventDispatch, this);
    this->_apIP      = WiFi.softAPIP();
    this->_apGateway = WiFi.softAPIP();
    this->_apSubnet  = WiFi.softAPSubnetMask();
}

void WifiEsp32S3Device::_reApIfActive() {
    if (this->_apActive) this->_softAP();
}

void WifiEsp32S3Device::_applyMode() {
    wifi_mode_t oldMode = WiFi.getMode();
    bool wasOff = (oldMode == WIFI_MODE_NULL || oldMode == WIFI_OFF);
    const char* modeStr = "OFF";
    bool nowOff = false;

    if (this->_apActive && this->_staActive) { WiFi.mode(WIFI_AP_STA); modeStr = "AP+STA"; }
    else if (this->_apActive)                { WiFi.mode(WIFI_AP);     modeStr = "AP";     }
    else if (this->_staActive)               { WiFi.mode(WIFI_STA);    modeStr = "STA";    }
    else { WiFi.mode(WIFI_OFF); nowOff = true; }

    Serial.printf("[Wifi] mode=%s\n", modeStr);

    if (wasOff != nowOff) {
        if (nowOff) {
            Serial.printf("[Wifi] -> chipWebServerRtos.rtosStop()\n");
            EASYB.chipWebServerRtos.rtosStop();
        } else {
            Serial.printf("[Wifi] -> chipWebServerRtos.rtosRun()\n");
            EASYB.chipWebServerRtos.rtosRun();
        }
    }
}

int WifiEsp32S3Device::_findClient(const char* mac) const {
    for (size_t i = 0; i < this->_apClients.size(); i++) {
        if (this->_apClients[i].equalsIgnoreCase(mac)) return i;
    }
    return -1;
}
