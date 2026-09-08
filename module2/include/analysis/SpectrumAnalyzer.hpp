#pragma once
#include <vector>

namespace module2 {
namespace analysis {

class SpectrumAnalyzer {
public:
    /**
     * @brief Computes the peak frequency from the PSD.
     * 
     * @param psd The Power Spectral Density values.
     * @param freqBins The corresponding frequency bins.
     * @return The frequency with the highest PSD magnitude.
     */
    static float computePeakFrequency(const std::vector<float>& psd, const std::vector<float>& freqBins);

    /**
     * @brief Computes the spectral centroid (center of mass).
     * 
     * @param psd The Power Spectral Density values.
     * @param freqBins The corresponding frequency bins.
     * @return The spectral centroid in Hz.
     */
    static float computeSpectralCentroid(const std::vector<float>& psd, const std::vector<float>& freqBins);

    /**
     * @brief Estimates the noise floor level from the PSD.
     * 
     * @param psd The Power Spectral Density values.
     * @return The estimated noise floor magnitude.
     */
    static float estimateNoiseFloor(const std::vector<float>& psd);
};

} // namespace analysis
} // namespace module2
