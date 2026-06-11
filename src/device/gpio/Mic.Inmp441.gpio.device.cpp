#include "core/base/DeviceBase.gpio.cpp"
#include "Mic.Inmp441.gpio.device.h"
#include "core/EasyBoard.h"

// ---- instance() ----
MicInmp441GpioDevice* MicInmp441GpioDevice::instance(const char* deviceName, gpio_num_t sck, gpio_num_t ws, gpio_num_t din) {
    static MicInmp441GpioDevice inst;
    inst.deviceName = deviceName;
    inst.PIN_GPIOS.sck.pin = sck;
    inst.PIN_GPIOS.ws.pin = ws;
    inst.PIN_GPIOS.din.pin = din;
    return &inst;
}

bool MicInmp441GpioDevice::boot(const char* serviceName) {
    if (!DeviceGpioBase::boot(serviceName)) return false;

    i2s_config_t cfg = {};
    cfg.mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
    cfg.sample_rate          = this->sampleRate;
    cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT;
    cfg.channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count        = 8;
    cfg.dma_buf_len          = 480;
    cfg.use_apll             = false;
    if (i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr) != ESP_OK) return false;

    i2s_pin_config_t i2sPin = {
        .bck_io_num   = this->PIN_GPIOS.sck.pin,
        .ws_io_num    = this->PIN_GPIOS.ws.pin,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = this->PIN_GPIOS.din.pin,
    };
    if (i2s_set_pin(I2S_NUM_0, &i2sPin) != ESP_OK) { unboot(serviceName); return false; }
    i2s_zero_dma_buffer(I2S_NUM_0);
    return true;
}

bool MicInmp441GpioDevice::read(std::vector<int16_t>& data, size_t samples) {
    std::vector<int32_t> buf32(samples);
    size_t bytesRead = 0;
    if (i2s_read(I2S_NUM_0, buf32.data(), samples * sizeof(int32_t), &bytesRead, pdMS_TO_TICKS(500)) != ESP_OK)
        return false;
    size_t n = bytesRead / sizeof(int32_t);
    data.resize(n);
    for (size_t i = 0; i < n; i++) {
        int32_t v = buf32[i] >> 12;
        data[i]  = (v > INT16_MAX) ? INT16_MAX : (v < -INT16_MAX) ? -INT16_MAX : (int16_t)v;
    }
    return !data.empty();
}

void MicInmp441GpioDevice::unboot(const char* serviceName) {
    if (this->bootServices.size() == 1) {
        i2s_driver_uninstall(I2S_NUM_0);
    }
    DeviceGpioBase::unboot(serviceName);
}
