#  RTL-SDR Advanced Band Scanner

<div align="center">

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![C++](https://img.shields.io/badge/C++-17-blue)
![Platform](https://img.shields.io/badge/platform-Linux-green)
![License](https://img.shields.io/badge/license-MIT-green)
![RTL-SDR](https://img.shields.io/badge/RTL--SDR-Compatible-red)

**spectrum monitoring and signal detection for RTL-SDR dongles**



</div>

---

##  Overview

The RTL-SDR Advanced Band Scanner is a professional-grade spectrum monitoring tool that automatically scans multiple frequency bands, detects active signals, classifies modulation types, and tracks signal persistence over time. Perfect for RF monitoring, amateur radio, spectrum analysis, and signal intelligence applications.If user need to monitor a specific frequency band, you can easily customize the configuration by editing bands.cpp according to your requirements.

### What it can do 

-  **Automated Band Scanning** - Sweeps through predefined frequency ranges
-  **Real-time Signal Detection** - Identifies signals above noise floor
-  **Signal Classification** - Automatically identifies modulation types (Can be changed based on user requirement)
-  **Persistence Tracking** - Eliminates false positives by tracking signals over multiple scans
-  **Signal Strength Ranking** - Shows strongest signals in current environment
-  **Bandwidth Measurement** - Calculates -3dB bandwidth for each signal
  

##  Supported Frequency Bands (User can change this based on there requirements)

| Band | Frequency Range | Step Size | Typical Modulation |
|------|----------------|-----------|-------------------|
|  FM Broadcast | 88.0 - 108.0 MHz | 2.0 MHz | FM |
|  Air Band | 118.0 - 137.0 MHz | 2.0 MHz | AM |
|  Weather Sat | 137.0 - 138.0 MHz | 1.0 MHz | WEFAX |
|  Ham 2m | 144.0 - 148.0 MHz | 2.0 MHz | NFM |
|  Marine VHF | 156.0 - 162.0 MHz | 2.0 MHz | NFM |
|  Ham 70cm | 420.0 - 450.0 MHz | 2.0 MHz | NFM/Digital |
|  PS | 450.0 - 470.0 MHz | 2.0 MHz | NFM/Digital |
|  Cellular Low | 700.0 - 800.0 MHz | 2.0 MHz | Digital |
|  Cellular High | 800.0 - 900.0 MHz | 2.0 MHz | Digital |
|  ISM/Ham 33cm | 902.0 - 928.0 MHz | 2.0 MHz | Digital/NFM |
|  ADS-B | 1090.0 MHz | 1.0 MHz | Pulse |

##  Installation

### Prerequisites

- RTL-SDR compatible dongle
- Linux operating system (Raspberry Pi, Ubuntu, Debian)
- Root/sudo privileges (for USB access)

### Install Dependencies

#### Ubuntu/Debian/Raspberry Pi OS

```bash
# Update package list
sudo apt-get update

# Install build tools
sudo apt-get install -y build-essential cmake git

# Install RTL-SDR library
sudo apt-get install -y librtlsdr-dev libusb-1.0-0-dev

# Install FFTW library
sudo apt-get install -y libfftw3-dev

# Install additional dependencies
sudo apt-get install -y pkg-config

```

```bash
# Clone the repository
git clone https://github.com/YOUR_USERNAME/rtl-sdr-band-scanner.git
cd rtl-sdr-band-scanner

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Compile
make -j$(nproc)

# The executable 'scanner' will be created in the build directory
