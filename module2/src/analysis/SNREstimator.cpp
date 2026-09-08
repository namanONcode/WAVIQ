#include "analysis/SNREstimator.hpp"
#include <cmath>

namespace module2 {
namespace analysis {

float SNREstimator::estimateSNR(const std::vector<float>& psd, float noiseFloor) {
    if (psd.empty() || noiseFloor <= 0.0f) {
        return 0.0f;
    }

    float signalPower = 0.0f;
    for (float val : psd) {
        // Assume anything above the noise floor (plus a margin) is signal
        if (val > noiseFloor * 1.5f) {
            signalPower += (val - noiseFloor);
        }
    }

    if (signalPower <= 0.0f) {
        return 0.0f; // Could be negative dB in reality, but 0.0f is a safe default
    }

    // Since PSD values are already power, SNR is 10 * log10(SignalPower / NoisePower)
    // Here we approximate NoisePower as noiseFloor * N (where N is the number of bins)
    float totalNoisePower = noiseFloor * psd.size();
    
    return 10.0f * std::log10(signalPower / totalNoisePower);
}

} // namespace analysis
} // namespace module2
