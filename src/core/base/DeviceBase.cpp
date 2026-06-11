#include "DeviceBase.h"

bool DeviceBase::boot(const char* serviceName) {
    for (auto s : this->bootServices)
        if (strcmp(s, serviceName) == 0) return false;
    bool first = this->bootServices.empty();
    this->bootServices.push_back(serviceName);
    if (first) this->booted = true;
    Serial.printf("[BOOT] %s <- '%s'  (wasBooting=%d, bootCount=%d)\n",
                  this->deviceName, serviceName, !first, (int)this->bootServices.size());
    return first;
}

void DeviceBase::unboot(const char* serviceName) {
    for (auto it = this->bootServices.begin(); it != this->bootServices.end(); ++it) {
        if (strcmp(*it, serviceName) == 0) { this->bootServices.erase(it); break; }
    }
    if (this->bootServices.empty()) this->booted = false;
    Serial.printf("[UNBOOT] %s  (remaining=%d)\n", this->deviceName, (int)this->bootServices.size());
}
