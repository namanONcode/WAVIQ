#pragma once
#include <vector>

namespace module2 {
namespace analysis {

class BandwidthEstimator {
public:
    /**
     * @brief Estimates the Occupied Bandwidth (OBW) using the 99% power rule.
     * 
     * @param psd The Power Spectral Density values.
     * @param freqBins The corresponding frequency bins.
     * @return The estimated 99% occupied bandwidth in Hz.
     */
    static float estimateOccupiedBandwidth(const std::vector<float>& psd, const std::vector<float>& freqBins);
};

} // namespace analysis
} // namespace module2
