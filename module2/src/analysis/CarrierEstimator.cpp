#include "analysis/CarrierEstimator.hpp"
#include <algorithm>
#include <cmath>

namespace module2 {
namespace analysis {

float CarrierEstimator::estimateCarrierOffset(const std::vector<float>& psd, const std::vector<float>& freqBins, const core::SignalData& data) {
    if (psd.empty() || freqBins.empty() || psd.size() != freqBins.size()) {
        return 0.0f;
    }

    // A simple method is to find the peak of the PSD.
    // More advanced methods would use phase derivatives.
    auto maxIt = std::max_element(psd.begin(), psd.end());
    size_t maxIdx = std::distance(psd.begin(), maxIt);
    
    return freqBins[maxIdx] - data.centerFrequency;
}

} // namespace analysis
} // namespace module2
