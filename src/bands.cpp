// bands.cpp
#include "sdr_scanner.h"

std::vector<Band> getBands() {
    return {
        {"FM Broadcast",    88.0e6,   108.0e6,   2.0e6,   -80.0, "FM"},
        {"Air Band",        118.0e6,  137.0e6,   2.0e6,   -90.0, "AM"},
        {"Weather Sat",     137.0e6,  138.0e6,   1.0e6,   -85.0, "WEFAX"},
        {"Ham 2m",         144.0e6,  148.0e6,   2.0e6,   -95.0, "NFM"},
        {"Marine VHF",      156.0e6,  162.0e6,   2.0e6,   -90.0, "NFM"},
        {"Ham 70cm",       420.0e6,  450.0e6,   2.0e6,   -95.0, "NFM/Digital"},
        {"Business/PS",    450.0e6,  470.0e6,   2.0e6,   -90.0, "NFM/Digital"},
        {"Cellular Low",   700.0e6,  800.0e6,   2.0e6,   -85.0, "Digital"},
        {"Cellular High",  800.0e6,  900.0e6,   2.0e6,   -85.0, "Digital"},
        {"ISM/Ham 33cm",   902.0e6,  928.0e6,   2.0e6,   -90.0, "Digital/NFM"},
        {"ADS-B",         1090.0e6, 1090.0e6,  1.0e6,   -85.0, "Pulse"}
    };
}
