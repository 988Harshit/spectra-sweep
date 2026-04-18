#ifndef SDR_SCANNER_H
#define SDR_SCANNER_H

#include <rtl-sdr.h>
#include <iostream>
#include <vector>
#include <complex>
#include <cmath>
#include <thread>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <map>
#include <set>
#include <mutex>
#include <fftw3.h>
#include <string>
#include <sstream>

// Configuration
#define SAMPLE_RATE 2400000      // 2.4 MHz
#define FFT_SIZE 8192            // Power of 2
#define NUM_FFTS_PER_CHUNK 5     // Average FFTs for stability
#define FREQ_TOLERANCE 5000      // 5 kHz for grouping
#define SIGNAL_THRESHOLD_DB 8.0  // dB above noise floor
#define PERSISTENCE_THRESHOLD 3   // Number of scans to confirm signal
#define HISTORY_SIZE 10          // Track last N scans

// Band definitions
struct Band {
    std::string name;
    double start_freq;
    double end_freq;
    double step_size;
    double min_power_dbfs;
    std::string modulation_type;
};

// Detected signal structure
struct DetectedSignal {
    double center_freq;
    double bandwidth_3db;
    double peak_power_dbfs;
    double snr_db;
    std::string type;
    int confidence;
    int persistence_count;
    int last_seen_scan;
    double max_power_history;
};

// Signal history for time tracking
struct SignalHistory {
    DetectedSignal current;
    std::vector<double> power_history;
    std::vector<double> freq_history;
    int times_seen;
    int times_missing;
    bool is_active;
};

// SDR Hardware class declaration
class SDRHardware {
private:
    rtlsdr_dev_t* dev;
    
public:
    SDRHardware();
    ~SDRHardware();
    bool initSDR(int device_index = 0);
    bool setFrequency(double center_freq);
    std::vector<uint8_t> captureSamples(int num_samples);
    double getSampleRate() const { return SAMPLE_RATE; }
};

// Signal Processing class declaration
class SignalProcessor {
private:
    fftwf_plan fft_plan;
    std::vector<std::complex<float>> fft_input;
    std::vector<std::complex<float>> fft_output;
    std::vector<float> window;
    int fft_size;
    
    void applyWindow();
    double getFrequencyFromBin(int bin, double center_freq);
    
public:
    SignalProcessor(int fft_size = FFT_SIZE);
    ~SignalProcessor();
    
    std::vector<float> computeFFT(const std::vector<uint8_t>& samples, double center_freq);
    std::vector<std::pair<double, double>> toSpectrum(const std::vector<float>& fft_power, double center_freq);
    double measureNoiseFloor(const std::vector<std::pair<double, double>>& spectrum);
};

// Signal Detector class declaration
class SignalDetector {
private:
    double signal_threshold_db;
    int freq_tolerance;
    
    double calculateBandwidth3dB(const std::vector<std::pair<double, double>>& spectrum, 
                                   double peak_freq, double peak_power);
    std::string classifySignal(double bandwidth_3db, double snr_db);
    int peakFreqIndex(const std::vector<std::pair<double, double>>& spectrum, double freq);
    
public:
    SignalDetector(double threshold = SIGNAL_THRESHOLD_DB, int tolerance = FREQ_TOLERANCE);
    
    std::vector<DetectedSignal> findSignals(const std::vector<std::pair<double, double>>& spectrum,
                                              double noise_floor);
    void mergeSignals(std::vector<DetectedSignal>& existing, const std::vector<DetectedSignal>& new_signals);
};

// Persistence Tracker class declaration
class PersistenceTracker {
private:
    std::map<double, SignalHistory> signal_database;
    std::mutex db_mutex;
    int current_scan_id;
    int persistence_threshold;
    
public:
    PersistenceTracker(int threshold = PERSISTENCE_THRESHOLD);
    
    void updateDatabase(const std::vector<DetectedSignal>& current_signals, int scan_id);
    void pruneInactiveSignals();
    int getActiveSignalCount();
    std::vector<std::pair<double, double>> getStrongestSignals(int count = 5);
    void printActivitySummary();
    const std::map<double, SignalHistory>& getDatabase() const { return signal_database; }
};

// Utility functions
std::string formatFrequency(double freq_hz);
std::string formatBandwidth(double bw_hz);
std::vector<Band> getBands();  // ← DECLARATION ADDED HERE

#endif // SDR_SCANNER_H
