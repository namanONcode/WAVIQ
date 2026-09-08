#include "analysis/BandwidthEstimator.hpp"
#include <numeric>
#include <cmath>

namespace module2 {
namespace analysis {

float BandwidthEstimator::estimateOccupiedBandwidth(const std::vector<float>& psd, const std::vector<float>& freqBins) {
    if (psd.empty() || freqBins.empty() || psd.size() != freqBins.size()) {
        return 0.0f;
    }

    // Calculate total power
    float totalPower = 0.0f;
    for (float val : psd) {
        totalPower += val;
    }

    if (totalPower == 0.0f) {
        return 0.0f;
    }

    float targetPowerLower = totalPower * 0.005f; // 0.5%
    float targetPowerUpper = totalPower * 0.995f; // 99.5%

    float currentPower = 0.0f;
    size_t lowerIdx = 0;
    size_t upperIdx = psd.size() - 1;

    for (size_t i = 0; i < psd.size(); ++i) {
        currentPower += psd[i];
        if (currentPower >= targetPowerLower) {
            lowerIdx = i;
            break;
        }
    }

    currentPower = 0.0f;
    for (size_t i = 0; i < psd.size(); ++i) {
        currentPower += psd[i];
        if (currentPower >= targetPowerUpper) {
            upperIdx = i;
            break;
        }
    }

    return std::abs(freqBins[upperIdx] - freqBins[lowerIdx]);
}

} // namespace analysis
} // namespace module2
