#include "sdr_scanner.h"

PersistenceTracker::PersistenceTracker(int threshold) 
    : current_scan_id(0), persistence_threshold(threshold) {}

void PersistenceTracker::updateDatabase(const std::vector<DetectedSignal>& current_signals, int scan_id) {
    std::lock_guard<std::mutex> lock(db_mutex);
    current_scan_id = scan_id;
    
    std::set<double> seen_freqs;
    
    for (const auto& sig : current_signals) {
        seen_freqs.insert(sig.center_freq);
        
        auto it = signal_database.find(sig.center_freq);
        if (it != signal_database.end()) {
            it->second.current = sig;
            it->second.times_seen++;
            it->second.times_missing = 0;
            it->second.is_active = true;
            it->second.power_history.push_back(sig.peak_power_dbfs);
            it->second.freq_history.push_back(sig.center_freq);
            
            if (it->second.power_history.size() > HISTORY_SIZE) {
                it->second.power_history.erase(it->second.power_history.begin());
                it->second.freq_history.erase(it->second.freq_history.begin());
            }
        } else {
            SignalHistory history;
            history.current = sig;
            history.times_seen = 1;
            history.times_missing = 0;
            history.is_active = true;
            history.power_history = {sig.peak_power_dbfs};
            history.freq_history = {sig.center_freq};
            signal_database[sig.center_freq] = history;
        }
    }
    
    for (auto& [freq, history] : signal_database) {
        if (seen_freqs.find(freq) == seen_freqs.end()) {
            history.times_missing++;
            if (history.times_missing >= persistence_threshold) {
                history.is_active = false;
            }
        }
    }
    
    pruneInactiveSignals();
}

void PersistenceTracker::pruneInactiveSignals() {
    for (auto it = signal_database.begin(); it != signal_database.end();) {
        if (!it->second.is_active && it->second.times_missing > 10) {
            it = signal_database.erase(it);
        } else {
            ++it;
        }
    }
}

int PersistenceTracker::getActiveSignalCount() {
    std::lock_guard<std::mutex> lock(db_mutex);
    int count = 0;
    for (const auto& [freq, history] : signal_database) {
        if (history.is_active) count++;
    }
    return count;
}

std::vector<std::pair<double, double>> PersistenceTracker::getStrongestSignals(int count) {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::vector<std::pair<double, double>> strongest;
    for (const auto& [freq, history] : signal_database) {
        if (history.is_active) {
            strongest.push_back({freq, history.current.peak_power_dbfs});
        }
    }
    
    std::sort(strongest.begin(), strongest.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    if (strongest.size() > count) {
        strongest.resize(count);
    }
    
    return strongest;
}

void PersistenceTracker::printActivitySummary() {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    int active_signals = 0;
    int persistent_signals = 0;
    
    for (const auto& [freq, history] : signal_database) {
        if (history.is_active) {
            active_signals++;
            if (history.times_seen >= persistence_threshold) {
                persistent_signals++;
            }
        }
    }
    
    std::cout << "\n📊 ACTIVITY SUMMARY" << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    
    if (active_signals == 0) {
        std::cout << "   No active signals detected" << std::endl;
    } else if (active_signals < 10) {
        std::cout << "   LIGHT ACTIVITY - " << active_signals << " signals" << std::endl;
    } else if (active_signals < 30) {
        std::cout << "   MODERATE ACTIVITY - " << active_signals << " signals" << std::endl;
    } else if (active_signals < 60) {
        std::cout << "   HIGH ACTIVITY - " << active_signals << " signals" << std::endl;
    } else {
        std::cout << "   *** VERY ACTIVE *** - " << active_signals << " signals" << std::endl;
    }
    
    std::cout << "   Persistent signals: " << persistent_signals << " (seen " 
              << persistence_threshold << "+ times)" << std::endl;
    
    // Top strongest signals
    auto strongest = getStrongestSignals(5);
    if (!strongest.empty()) {
        std::cout << "\n💪 Strongest signals (dBFS):" << std::endl;
        for (const auto& sig : strongest) {
            std::cout << "   " << formatFrequency(sig.first) 
                      << " @ " << std::fixed << std::setprecision(1) 
                      << sig.second << " dBFS" << std::endl;
        }
    }
}
