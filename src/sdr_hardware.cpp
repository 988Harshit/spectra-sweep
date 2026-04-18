#include "sdr_scanner.h"

SDRHardware::SDRHardware() : dev(nullptr) {}

SDRHardware::~SDRHardware() {
    if (dev) {
        rtlsdr_close(dev);
    }
}

bool SDRHardware::initSDR(int device_index) {
    int result = rtlsdr_open(&dev, device_index);
    if (result < 0) {
        std::cerr << "Failed to open RTL-SDR device: " << result << std::endl;
        return false;
    }
    
    rtlsdr_set_sample_rate(dev, SAMPLE_RATE);
    rtlsdr_set_tuner_gain_mode(dev, 1);
    
    int gains[100];
    int num_gains = rtlsdr_get_tuner_gains(dev, gains);
    if (num_gains > 0) {
        rtlsdr_set_tuner_gain(dev, gains[num_gains - 1]);
        std::cout << "Gain set to: " << gains[num_gains - 1] / 10.0 << " dB" << std::endl;
    } else {
        rtlsdr_set_tuner_gain_mode(dev, 0);
        std::cout << "Using automatic gain" << std::endl;
    }
    
    rtlsdr_set_freq_correction(dev, 0);
    rtlsdr_reset_buffer(dev);
    
    std::cout << "SDR initialized successfully!" << std::endl;
    std::cout << "Sample rate: " << SAMPLE_RATE / 1e6 << " MHz" << std::endl;
    std::cout << "FFT size: " << FFT_SIZE << " points" << std::endl;
    
    return true;
}

bool SDRHardware::setFrequency(double center_freq) {
    uint32_t freq_hz = static_cast<uint32_t>(center_freq);
    int result = rtlsdr_set_center_freq(dev, freq_hz);
    if (result < 0) {
        std::cerr << "Failed to set center frequency" << std::endl;
        return false;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return true;
}

std::vector<uint8_t> SDRHardware::captureSamples(int num_samples) {
    std::vector<uint8_t> buffer(num_samples * 2);
    int n_read = 0;
    
    int result = rtlsdr_read_sync(dev, buffer.data(), buffer.size(), &n_read);
    if (result < 0 || n_read < static_cast<int>(buffer.size())) {
        return std::vector<uint8_t>();
    }
    
    return buffer;
}
