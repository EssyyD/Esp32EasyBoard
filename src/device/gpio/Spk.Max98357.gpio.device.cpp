#include "core/base/DeviceBase.gpio.cpp"
#include "Spk.Max98357.gpio.device.h"
#include "core/EasyBoard.h"

// ---- instance() ----
SpkMax98357GpioDevice* SpkMax98357GpioDevice::instance(const char* deviceName, gpio_num_t bclk, gpio_num_t lrck, gpio_num_t dout) {
    static SpkMax98357GpioDevice inst;
    inst.deviceName = deviceName;
    inst.PIN_GPIOS.bclk.pin = bclk;
    inst.PIN_GPIOS.lrck.pin = lrck;
    inst.PIN_GPIOS.dout.pin = dout;
    return &inst;
}

bool SpkMax98357GpioDevice::boot(const char* serviceName) {
    if (!DeviceGpioBase::boot(serviceName)) return false;

    i2s_config_t cfg = {};
    cfg.mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    cfg.sample_rate          = this->sampleRate;
    cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT;
    cfg.channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count        = 8;
    cfg.dma_buf_len          = 480;
    cfg.use_apll             = false;
    if (i2s_driver_install(I2S_NUM_1, &cfg, 0, nullptr) != ESP_OK) return false;

    i2s_pin_config_t i2sPin = {
        .bck_io_num   = this->PIN_GPIOS.bclk.pin,
        .ws_io_num    = this->PIN_GPIOS.lrck.pin,
        .data_out_num = this->PIN_GPIOS.dout.pin,
        .data_in_num  = I2S_PIN_NO_CHANGE,
    };
    if (i2s_set_pin(I2S_NUM_1, &i2sPin) != ESP_OK) { unboot(serviceName); return false; }
    i2s_zero_dma_buffer(I2S_NUM_1);
    return true;
}

void SpkMax98357GpioDevice::setDeviceVolume(int vol) {
    vol = (vol < 0) ? 0 : (vol > 100) ? 100 : vol;
    this->volFactor = (int32_t)(std::pow((double)vol / 100.0, 2) * 65536);
}

bool SpkMax98357GpioDevice::write(const std::vector<int16_t>& data) {
    if (data.empty()) return false;
    std::vector<int32_t> buf32(data.size());
    for (size_t i = 0; i < data.size(); i++) {
        int64_t val = int64_t(data[i]) * this->volFactor;
        buf32[i] = (val > INT32_MAX) ? INT32_MAX : (val < INT32_MIN) ? INT32_MIN : (int32_t)val;
    }
    size_t written = 0;
    return i2s_write(I2S_NUM_1, buf32.data(), buf32.size() * sizeof(int32_t), &written, portMAX_DELAY) == ESP_OK;
}

void SpkMax98357GpioDevice::unboot(const char* serviceName) {
    if (this->bootServices.size() == 1) {
        i2s_driver_uninstall(I2S_NUM_1);
    }
    DeviceGpioBase::unboot(serviceName);
}


