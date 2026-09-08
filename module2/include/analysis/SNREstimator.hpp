#pragma once
#include <vector>

namespace module2 {
namespace analysis {

class SNREstimator {
public:
    /**
     * @brief Estimates the Signal-to-Noise Ratio (SNR) in dB.
     * 
     * @param psd The Power Spectral Density values.
     * @param noiseFloor The estimated noise floor level.
     * @return The estimated SNR in dB.
     */
    static float estimateSNR(const std::vector<float>& psd, float noiseFloor);
};

} // namespace analysis
} // namespace module2
