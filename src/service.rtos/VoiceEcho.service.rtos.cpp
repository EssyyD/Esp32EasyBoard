#include "core/base/DeviceBase.gpio.cpp"
#include "core/EasyBoard.h"
#include "VoiceEcho.service.rtos.h"

#include <Arduino.h>

void VoiceEchoServiceRtos::_rtosBegin() {
    this->rec = new Recorder();

    EASYB.devicesI2c.oledSsd13065->boot(this->serviceName);

    EASYB.devicesGpio.mic->setDeviceSampleRate(DEVICE_SET_VOICE_ECHO_MIC_SAMPLE_RATE);
    if (!EASYB.devicesGpio.mic->boot(this->serviceName)) {
        Serial.println("[FATAL] Mic boot failed");
        EASYB.devicesI2c.oledSsd13065->print("Mic FAIL");
        EASYB.setLed(255, 0, 0);
        while (1) delay(1000);
    }
    Serial.printf("[OK]  Mic    fs=%d\n", DEVICE_SET_VOICE_ECHO_MIC_SAMPLE_RATE);
    delay(200);

    EASYB.devicesGpio.spk->setDeviceSampleRate(DEVICE_SET_VOICE_ECHO_SPK_SAMPLE_RATE);
    EASYB.devicesGpio.spk->setDeviceVolume(DEVICE_SET_VOICE_ECHO_SPK_VOLUME);
    if (!EASYB.devicesGpio.spk->boot(this->serviceName)) {
        Serial.println("[FATAL] Spk boot failed");
        EASYB.devicesI2c.oledSsd13065->print("Spk FAIL");
        EASYB.setLed(255, 0, 0);
        while (1) delay(1000);
    }
    Serial.printf(
        "[OK]  Spk    fs=%d  vol=%d%%\n",
        DEVICE_SET_VOICE_ECHO_SPK_SAMPLE_RATE,
        DEVICE_SET_VOICE_ECHO_SPK_VOLUME
    );
    EASYB.devicesI2c.oledSsd13065->print("Listening...");
    EASYB.setLed(0, 255, 0);
    Serial.printf(
        "=== thr=%.0f%% sil=%ds pre=%dms db=%dx%dms ===\n",
        DEVICE_SET_VOICE_ECHO_VAD_THRESHOLD * 100,
        DEVICE_SET_VOICE_ECHO_SILENCE_SEC,
        DEVICE_SET_VOICE_ECHO_PREROLL_MS,
        DEVICE_SET_VOICE_ECHO_VAD_DEBOUNCE,
        30
    );
}

void VoiceEchoServiceRtos::_rtosEnd() {
    EASYB.devicesI2c.oledSsd13065->unboot(this->serviceName);
    EASYB.devicesGpio.spk->unboot(this->serviceName);
    EASYB.devicesGpio.mic->unboot(this->serviceName);
}

bool VoiceEchoServiceRtos::isLoud(const std::vector<int16_t>& chunk) {
    for (auto s : chunk)
        if (std::abs(s) / 32768.0f > DEVICE_SET_VOICE_ECHO_VAD_THRESHOLD) return true;
    return false;
}

void VoiceEchoServiceRtos::_rtosLoop() {
    if (!this->rec) return;
    static int dot = 0;

    int prerollSamples = DEVICE_SET_VOICE_ECHO_MIC_SAMPLE_RATE * DEVICE_SET_VOICE_ECHO_PREROLL_MS / 1000;
    int maxSamples = DEVICE_SET_VOICE_ECHO_MIC_SAMPLE_RATE * DEVICE_SET_VOICE_ECHO_MAX_RECORD_SEC;

    std::vector<int16_t> chunk;
    if (!EASYB.devicesGpio.mic->read(chunk, 480) || chunk.empty()) return;

    if (!this->rec->recording) {
        this->rec->preroll.insert(this->rec->preroll.end(), chunk.begin(), chunk.end());
        while ((int)this->rec->preroll.size() > prerollSamples) {
            auto p = this->rec->preroll.begin();
            this->rec->preroll.erase(
                p,
                p + std::min(
                    (int)chunk.size(),
                    (int)this->rec->preroll.size()
                )
            );
        }
        if (this->isLoud(chunk)) {
            this->rec->loudStreak++;
            if (this->rec->loudStreak >= DEVICE_SET_VOICE_ECHO_VAD_DEBOUNCE) {
                this->rec->buffer.clear();
                this->rec->buffer.reserve(maxSamples + prerollSamples);
                this->rec->buffer.insert(this->rec->buffer.end(), this->rec->preroll.begin(), this->rec->preroll.end());
                this->rec->preroll.clear();
                this->rec->recording      = true;
                this->rec->silenceSamples = 0;
                this->rec->loudStreak     = 0;
                this->rec->buffer.insert(this->rec->buffer.end(), chunk.begin(), chunk.end());
                dot = 0;
                EASYB.setLed(255, 255, 0);
                EASYB.devicesI2c.oledSsd13065->print("* Recording *");
                Serial.print(".");
            }
        } else {
            this->rec->loudStreak = 0;
        }
    } else {
        this->rec->buffer.insert(this->rec->buffer.end(), chunk.begin(), chunk.end());
        if (++dot % 40 == 0) Serial.print(".");

        float sum = 0;
        for (auto s : chunk) sum += std::abs(s) / 32768.0f;
        float avg = sum / chunk.size();
        if (avg < DEVICE_SET_VOICE_ECHO_VAD_THRESHOLD) this->rec->silenceSamples += chunk.size();
        else                                            this->rec->silenceSamples  = 0;

        bool ended = false;
        if ((int)this->rec->buffer.size() >= maxSamples) {
            Serial.println("[timeout]"); ended = true;
        }
        if (this->rec->silenceSamples >= DEVICE_SET_VOICE_ECHO_SILENCE_SEC * DEVICE_SET_VOICE_ECHO_MIC_SAMPLE_RATE) {
            Serial.printf(
                "[silence] %.1fs\n",
                (float)this->rec->silenceSamples / DEVICE_SET_VOICE_ECHO_MIC_SAMPLE_RATE
            );
            ended = true;
        }
        if (ended) {
            this->rec->recording = false;
            size_t total = this->rec->buffer.size();
            Serial.printf(
                "\n[end] %zu samples (%.1fs)\n",
                total,
                (float)total / DEVICE_SET_VOICE_ECHO_MIC_SAMPLE_RATE
            );
            if (total < (size_t)(DEVICE_SET_VOICE_ECHO_MIC_SAMPLE_RATE * 0.3f)) {
                Serial.println("[drop] too short");
                EASYB.devicesI2c.oledSsd13065->print("Listening...");
                EASYB.setLed(0, 255, 0);
                return;
            }
            EASYB.devicesI2c.oledSsd13065->print("* Playing *");
            EASYB.setLed(0, 255, 255);
            Serial.print("[play] "); delay(80);
            auto buf = this->rec->buffer.begin();
            for (size_t i = 0; i < total; i += 960) {
                size_t n = std::min<size_t>(960, total - i);
                std::vector<int16_t> out(buf + i, buf + i + n);
                EASYB.devicesGpio.spk->write(out);
            }
            std::vector<int16_t> silence(960, 0);
            for (int i = 0; i < 4; i++) EASYB.devicesGpio.spk->write(silence);
            EASYB.devicesI2c.oledSsd13065->print("Listening...");
            EASYB.setLed(0, 255, 0);
            Serial.println("done");
            delay(400);
        }
    }
    return;
}
