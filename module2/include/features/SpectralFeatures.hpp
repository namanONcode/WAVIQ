#pragma once
#include <vector>

namespace module2 {
namespace features {

/**
 * @brief Computes spectral-domain features from a pre-computed PSD.
 *
 * These features capture the spectral shape and are useful for
 * distinguishing narrowband vs wideband modulations.
 */
struct SpectralStats {
    float centroid;     ///< Spectral centroid (center of mass, Hz)
    float spread;       ///< Spectral spread (sqrt of 2nd central moment, Hz)
    float flatness;     ///< Spectral flatness: geometric_mean(PSD) / arithmetic_mean(PSD)
    float rolloff;      ///< Spectral rolloff: freq below which 85% of energy resides (Hz)
    float entropy;      ///< Spectral entropy (normalized, 0 = pure tone, 1 = white noise)
    float peakToMean;   ///< Peak PSD / mean PSD ratio
};

class SpectralFeatures {
public:
    /**
     * @brief Computes spectral features from a PSD and frequency bins.
     * @param psd The Power Spectral Density values.
     * @param freqBins The corresponding frequency bins (Hz).
     * @return SpectralStats populated with computed features.
     */
    static SpectralStats compute(const std::vector<float>& psd, const std::vector<float>& freqBins);
};

} // namespace features
} // namespace module2
