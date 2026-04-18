#include "sdr_scanner.h"

SignalProcessor::SignalProcessor(int fft_size) : fft_size(fft_size) {
    fft_input.resize(fft_size);
    fft_output.resize(fft_size);
    
    // Create Hann window
    window.resize(fft_size);
    for (int i = 0; i < fft_size; i++) {
        window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (fft_size - 1)));
    }
    
    // Create FFT plan
    fft_plan = fftwf_plan_dft_1d(
        fft_size,
        reinterpret_cast<fftwf_complex*>(fft_input.data()),
        reinterpret_cast<fftwf_complex*>(fft_output.data()),
        FFTW_FORWARD,
        FFTW_ESTIMATE
    );
}

SignalProcessor::~SignalProcessor() {
    if (fft_plan) {
        fftwf_destroy_plan(fft_plan);
    }
    fftwf_cleanup();
}

void SignalProcessor::applyWindow() {
    for (int i = 0; i < fft_size; i++) {
        fft_input[i] *= window[i];
    }
}

double SignalProcessor::getFrequencyFromBin(int bin, double center_freq) {
    double freq_resolution = static_cast<double>(SAMPLE_RATE) / fft_size;
    double freq_offset;
    
    if (bin < fft_size / 2) {
        freq_offset = bin * freq_resolution;
    } else {
        freq_offset = (bin - fft_size) * freq_resolution;
    }
    
    return center_freq + freq_offset;
}

std::vector<float> SignalProcessor::computeFFT(const std::vector<uint8_t>& samples, double center_freq) {
    std::vector<float> avg_power(fft_size, 0.0f);
    
    for (int fft_num = 0; fft_num < NUM_FFTS_PER_CHUNK; fft_num++) {
        // Convert I/Q samples to float complex and apply window
        for (int i = 0; i < fft_size; i++) {
            uint8_t I_byte = samples[i * 2];
            uint8_t Q_byte = samples[i * 2 + 1];
            
            float I = (static_cast<float>(I_byte) - 127.5f) / 128.0f;
            float Q = (static_cast<float>(Q_byte) - 127.5f) / 128.0f;
            
            fft_input[i] = std::complex<float>(I, Q) * window[i];
        }
        
        // Perform FFT
        fftwf_execute(fft_plan);
        
        // Calculate power for all bins
        for (int i = 0; i < fft_size; i++) {
            float real = fft_output[i].real();
            float imag = fft_output[i].imag();
            float power = (real * real + imag * imag) / fft_size;
            float power_dbfs = 10.0f * log10(power + 1e-12);
            
            avg_power[i] += power_dbfs;
        }
    }
    
    // Average the FFTs
    for (auto& p : avg_power) {
        p /= NUM_FFTS_PER_CHUNK;
    }
    
    return avg_power;
}

std::vector<std::pair<double, double>> SignalProcessor::toSpectrum(const std::vector<float>& fft_power, double center_freq) {
    std::vector<std::pair<double, double>> results;
    results.reserve(fft_size);
    
    for (int i = 0; i < fft_size; i++) {
        double absolute_freq = getFrequencyFromBin(i, center_freq);
        results.push_back({absolute_freq, fft_power[i]});
    }
    
    std::sort(results.begin(), results.end());
    return results;
}

double SignalProcessor::measureNoiseFloor(const std::vector<std::pair<double, double>>& spectrum) {
    if (spectrum.empty()) return -100.0;
    
    std::vector<double> powers;
    for (const auto& point : spectrum) {
        powers.push_back(point.second);
    }
    
    std::sort(powers.begin(), powers.end());
    size_t median_idx = powers.size() / 2;
    
    return powers[median_idx];
}

// SignalDetector implementation
SignalDetector::SignalDetector(double threshold, int tolerance) 
    : signal_threshold_db(threshold), freq_tolerance(tolerance) {}

int SignalDetector::peakFreqIndex(const std::vector<std::pair<double, double>>& spectrum, double freq) {
    for (size_t i = 0; i < spectrum.size(); i++) {
        if (std::abs(spectrum[i].first - freq) < 100) {
            return i;
        }
    }
    return spectrum.size() / 2;
}

double SignalDetector::calculateBandwidth3dB(const std::vector<std::pair<double, double>>& spectrum, 
                                               double peak_freq, double peak_power) {
    double target_power = peak_power - 3.0;
    double left_freq = peak_freq;
    double right_freq = peak_freq;
    
    int peak_idx = peakFreqIndex(spectrum, peak_freq);
    
    // Find left -3 dB point
    for (int i = peak_idx; i >= 0; i--) {
        if (spectrum[i].second <= target_power) {
            left_freq = spectrum[i].first;
            break;
        }
    }
    
    // Find right -3 dB point
    for (size_t i = peak_idx; i < spectrum.size(); i++) {
        if (spectrum[i].second <= target_power) {
            right_freq = spectrum[i].first;
            break;
        }
    }
    
    return right_freq - left_freq;
}

std::string SignalDetector::classifySignal(double bandwidth_3db, double snr_db) {
    if (bandwidth_3db < 25000) {
        if (snr_db > 20) return "NBFM Voice";
        else if (snr_db > 12) return "Digital (DMR/P25)";
        else return "Narrowband Signal";
    } else if (bandwidth_3db > 100000 && bandwidth_3db < 300000) {
        return "FM Broadcast";
    } else if (bandwidth_3db > 500000) {
        return "Wideband Digital";
    } else if (bandwidth_3db > 8000 && bandwidth_3db < 20000) {
        return "AM Voice";
    } else {
        return "Unknown Signal";
    }
}

std::vector<DetectedSignal> SignalDetector::findSignals(const std::vector<std::pair<double, double>>& spectrum,
                                                          double noise_floor) {
    std::vector<DetectedSignal> signals;
    std::vector<std::pair<double, double>> peaks;
    
    double threshold = noise_floor + signal_threshold_db;
    
    // Find peaks
    for (size_t i = 1; i < spectrum.size() - 1; i++) {
        if (spectrum[i].second > threshold &&
            spectrum[i].second > spectrum[i-1].second &&
            spectrum[i].second > spectrum[i+1].second) {
            peaks.push_back(spectrum[i]);
        }
    }
    
    if (peaks.empty()) return signals;
    
    // Group nearby peaks
    std::vector<std::vector<std::pair<double, double>>> signal_groups;
    signal_groups.push_back({peaks[0]});
    
    for (size_t i = 1; i < peaks.size(); i++) {
        if (peaks[i].first - peaks[i-1].first < freq_tolerance) {
            signal_groups.back().push_back(peaks[i]);
        } else {
            signal_groups.push_back({peaks[i]});
        }
    }
    
    // Process each group
    for (const auto& group : signal_groups) {
        if (group.empty()) continue;
        
        DetectedSignal signal;
        
        // Find peak with maximum power
        auto max_peak = *std::max_element(group.begin(), group.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        
        signal.center_freq = max_peak.first;
        signal.peak_power_dbfs = max_peak.second;
        signal.snr_db = max_peak.second - noise_floor;
        
        // Calculate -3 dB bandwidth
        signal.bandwidth_3db = calculateBandwidth3dB(spectrum, signal.center_freq, signal.peak_power_dbfs);
        
        // Classify the signal
        signal.type = classifySignal(signal.bandwidth_3db, signal.snr_db);
        
        signal.confidence = std::min(95, static_cast<int>(signal.snr_db * 5));
        signal.persistence_count = 1;
        signal.last_seen_scan = 0;
        signal.max_power_history = signal.peak_power_dbfs;
        
        signals.push_back(signal);
    }
    
    return signals;
}

void SignalDetector::mergeSignals(std::vector<DetectedSignal>& existing, const std::vector<DetectedSignal>& new_signals) {
    for (const auto& new_sig : new_signals) {
        bool merged = false;
        
        for (auto& existing_sig : existing) {
            if (std::abs(existing_sig.center_freq - new_sig.center_freq) < freq_tolerance) {
                existing_sig.peak_power_dbfs = std::max(existing_sig.peak_power_dbfs, new_sig.peak_power_dbfs);
                existing_sig.snr_db = std::max(existing_sig.snr_db, new_sig.snr_db);
                existing_sig.confidence = std::max(existing_sig.confidence, new_sig.confidence);
                existing_sig.bandwidth_3db = (existing_sig.bandwidth_3db + new_sig.bandwidth_3db) / 2;
                existing_sig.persistence_count++;
                merged = true;
                break;
            }
        }
        
        if (!merged) {
            existing.push_back(new_sig);
        }
    }
}
