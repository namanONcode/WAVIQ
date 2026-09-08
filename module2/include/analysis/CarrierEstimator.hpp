#pragma once
#include "core/SignalData.hpp"
#include <vector>

namespace module2 {
namespace analysis {

class CarrierEstimator {
public:
    /**
     * @brief Estimates the carrier frequency offset of the baseband signal.
     * 
     * @param psd The Power Spectral Density values.
     * @param freqBins The corresponding frequency bins.
     * @param data The time-domain signal data (optional for phase derivative methods).
     * @return The estimated carrier frequency offset in Hz.
     */
    static float estimateCarrierOffset(const std::vector<float>& psd, const std::vector<float>& freqBins, const core::SignalData& data);
};

} // namespace analysis
} // namespace module2
