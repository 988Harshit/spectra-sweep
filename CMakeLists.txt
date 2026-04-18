cmake_minimum_required(VERSION 3.10)
project(SDRScanner VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(RTLSDR REQUIRED librtlsdr)
pkg_check_modules(FFTW3F REQUIRED fftw3f)

# Add pthread (threading library)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${RTLSDR_INCLUDE_DIRS})
include_directories(${FFTW3F_INCLUDE_DIRS})

# Collect source files
set(SOURCES
    src/main.cpp
    src/sdr_hardware.cpp
    src/signal_detection.cpp
    src/persistence.cpp
    src/bands.cpp
)

# Create executable
add_executable(scanner ${SOURCES})

# Link libraries
target_link_libraries(scanner
    ${RTLSDR_LIBRARIES}
    ${FFTW3F_LIBRARIES}
    Threads::Threads
    m
)

# Optional: Add compiler warnings
target_compile_options(scanner PRIVATE -Wall -Wextra)

# Optional: Set optimization level
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
