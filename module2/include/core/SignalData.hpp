#pragma once
#include <vector>
#include <complex>

namespace module2 {
namespace core {

struct SignalData {
    // We use std::complex<float> as the default for IQ samples
    std::vector<std::complex<float>> samples;
    
    // Core parameters
    double sampleRate;      // Fs in Hz
    double centerFrequency; // Fc in Hz
};

} // namespace core
} // namespace module2
