#pragma once
#include "core/SignalData.hpp"
#include <vector>

namespace module2 {
namespace analysis {

class FFTAnalyzer {
public:
    /**
     * @brief Computes the Power Spectral Density (PSD) of the input signal.
     * 
     * Applies a windowing function (e.g. Hann) to the signal, computes the FFT,
     * and returns the squared magnitude of the FFT bins.
     * Also returns the corresponding frequency bins.
     * 
     * @param data The input signal data.
     * @param psd Output vector to store the computed PSD values.
     * @param freqBins Output vector to store the corresponding frequency for each PSD bin.
     */
    static void computePSD(const core::SignalData& data, std::vector<float>& psd, std::vector<float>& freqBins);
};

} // namespace analysis
} // namespace module2
