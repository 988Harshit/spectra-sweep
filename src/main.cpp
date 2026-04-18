#include "sdr_scanner.h"
#include <thread>
#include <chrono>

// Utility function implementations
std::string formatFrequency(double freq_hz) {
    std::stringstream ss;
    if (freq_hz >= 1e6) {
        ss << std::fixed << std::setprecision(4) << freq_hz / 1e6 << " MHz";
    } else if (freq_hz >= 1e3) {
        ss << std::fixed << std::setprecision(1) << freq_hz / 1e3 << " kHz";
    } else {
        ss << std::fixed << std::setprecision(0) << freq_hz << " Hz";
    }
    return ss.str();
}

std::string formatBandwidth(double bw_hz) {
    std::stringstream ss;
    if (bw_hz >= 1000) {
        ss << std::fixed << std::setprecision(1) << bw_hz / 1000 << " kHz";
    } else {
        ss << std::fixed << std::setprecision(0) << bw_hz << " Hz";
    }
    return ss.str();
}

void printBanner() {
    printf("  ____  _                       _   _____ _                   \n");
    printf(" / ___|| |_ _ __ ___  ___  ___| | |_   _| |__   ___ _ __ ___ \n");
    printf(" \\___ \\| __| '__/ _ \\/ __|/ _ \\ |   | | | '_ \\ / _ \\ '__/ _ \\\n");
    printf("  ___) | |_| | | (_) \\__ \\  __/ |   | | | | | |  __/ | |  __/\n");
    printf(" |____/ \\__|_|  \\___/|___/\\___|_|   |_| |_| |_|\\___|_|  \\___|\n");
    printf("\n");
    printf("  ==========================================================\n");
    printf("                     SIGNAL FLOW SYSTEM  V1                     \n");
    printf("  ==========================================================\n");
    printf("\n");
}

void printResults(const std::vector<DetectedSignal>& signals, const Band& band) {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "Band: " << band.name << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    if (signals.empty()) {
        std::cout << "   No signals detected above threshold" << std::endl;
        return;
    }
    
    auto sorted_signals = signals;
    std::sort(sorted_signals.begin(), sorted_signals.end(),
              [](const DetectedSignal& a, const DetectedSignal& b) {
                  return a.center_freq < b.center_freq;
              });
    
    std::cout << std::left 
              << std::setw(15) << "Frequency"
              << std::setw(14) << "Power (dBFS)"
              << std::setw(10) << "SNR (dB)"
              << std::setw(15) << "Bandwidth (-3dB)"
              << std::setw(20) << "Type"
              << std::setw(10) << "Confidence"
              << std::endl;
    std::cout << std::string(84, '-') << std::endl;
    
    for (const auto& signal : sorted_signals) {
        std::cout << std::left
                  << std::setw(15) << formatFrequency(signal.center_freq)
                  << std::setw(14) << std::fixed << std::setprecision(1) << signal.peak_power_dbfs
                  << std::setw(10) << std::setprecision(1) << signal.snr_db
                  << std::setw(15) << formatBandwidth(signal.bandwidth_3db)
                  << std::setw(20) << signal.type
                  << std::setw(10) << std::to_string(signal.confidence) + "%"
                  << std::endl;
    }
    
    std::cout << "\n Total signals found: " << signals.size() << std::endl;
}

std::vector<DetectedSignal> scanBand(const Band& band, SDRHardware& sdr, 
                                      SignalProcessor& processor, SignalDetector& detector) {
    std::cout << "\n  Scanning " << band.name << " [" 
              << band.start_freq / 1e6 << " - " 
              << band.end_freq / 1e6 << " MHz]..." << std::endl;
    
    std::vector<DetectedSignal> all_signals;
    double current_freq = band.start_freq;
    
    while (current_freq <= band.end_freq) {
        double center_freq = current_freq + (SAMPLE_RATE / 4);
        if (center_freq > band.end_freq - (SAMPLE_RATE / 4)) {
            center_freq = band.end_freq - (SAMPLE_RATE / 4);
        }
        
        if (!sdr.setFrequency(center_freq)) {
            current_freq += band.step_size;
            continue;
        }
        
        auto samples = sdr.captureSamples(FFT_SIZE);
        if (samples.empty()) {
            current_freq += band.step_size;
            continue;
        }
        
        auto fft_power = processor.computeFFT(samples, center_freq);
        auto spectrum = processor.toSpectrum(fft_power, center_freq);
        
        // Filter to band boundaries
        std::vector<std::pair<double, double>> band_spectrum;
        for (const auto& point : spectrum) {
            if (point.first >= band.start_freq && point.first <= band.end_freq) {
                band_spectrum.push_back(point);
            }
        }
        
        double noise_floor = processor.measureNoiseFloor(band_spectrum);
        auto signals = detector.findSignals(band_spectrum, noise_floor);
        
        detector.mergeSignals(all_signals, signals);
        
        // Progress indicator
        static int step_count = 0;
        if (++step_count % 5 == 0) {
            double progress = (current_freq - band.start_freq) / (band.end_freq - band.start_freq) * 100;
            std::cout << "\rProgress: " << std::fixed << std::setprecision(1) << progress << "%" << std::flush;
        }
        
        current_freq += band.step_size;
    }
    
    std::cout << "\r Scan complete!                      " << std::endl;
    
    return all_signals;
}

int main(int argc, char* argv[]) {
    // Print the banner
    printBanner();
    
    // Wait for 1 minute (60 seconds)
    std::cout << "System initializing... Waiting 5 seconds before starting scan..." << std::endl;
    std::cout << "   Press Ctrl+C to cancel\n" << std::endl;
    
    for (int i = 5; i > 0; i--) {
        std::cout << "\r   Countdown: " << i << " seconds remaining..." << std::flush;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "\r   Starting scan now!                    \n" << std::endl;
    
    std::cout << "\n  RTL-SDR Advanced Band Scanner with Persistence" << std::endl;
    std::cout << "================================================" << std::endl;
    
    SDRHardware sdr;
    SignalProcessor processor(FFT_SIZE);
    SignalDetector detector(SIGNAL_THRESHOLD_DB, FREQ_TOLERANCE);
    PersistenceTracker tracker(PERSISTENCE_THRESHOLD);
    
    int device_index = (argc > 1) ? std::atoi(argv[1]) : 0;
    
    if (!sdr.initSDR(device_index)) {
        std::cerr << "   Failed to initialize SDR" << std::endl;
        std::cerr << "   Make sure RTL-SDR is connected and you have permissions" << std::endl;
        return -1;
    }
    
    auto bands = getBands();  // This now calls getBands() from bands.cpp
    auto start_time = std::chrono::steady_clock::now();
    
    for (int scan_pass = 0; scan_pass < 3; scan_pass++) {
        std::cout << "\n  Scan Pass " << (scan_pass + 1) << "/3" << std::endl;
        std::cout << std::string(40, '-') << std::endl;
        
        for (const auto& band : bands) {
            auto signals = scanBand(band, sdr, processor, detector);
            tracker.updateDatabase(signals, scan_pass);
            printResults(signals, band);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        tracker.printActivitySummary();
        
        if (scan_pass < 2) {
            std::cout << "\n  Waiting 5 seconds before next scan pass..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "   SCAN COMPLETE" << std::endl;
    std::cout << "   Total time: " << duration.count() << " seconds" << std::endl;
    std::cout << "   Active signals: " << tracker.getActiveSignalCount() << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    return 0;
}
