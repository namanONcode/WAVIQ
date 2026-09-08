#pragma once
#include "core/SignalData.hpp"
#include <vector>

namespace module2 {
namespace features {

/**
 * @brief Computes instantaneous frequency features from IQ samples.
 *
 * Instantaneous frequency is estimated via the phase derivative:
 *   f[n] = (phi[n] - phi[n-1]) * Fs / (2π)
 * where phi[n] = atan2(Q[n], I[n]), with unwrapping.
 */
struct FrequencyStats {
    float mean;         ///< Mean instantaneous frequency (Hz)
    float variance;     ///< Variance (Hz^2)
    float stddev;       ///< Standard deviation (Hz)
    float skewness;     ///< 3rd standardized moment
    float kurtosis;     ///< Excess kurtosis
};

class FrequencyFeatures {
public:
    /**
     * @brief Extracts instantaneous frequency statistics.
     * @param data Input IQ signal.
     * @return FrequencyStats populated with computed features.
     */
    static FrequencyStats compute(const core::SignalData& data);

    /**
     * @brief Computes the instantaneous frequency vector.
     * @param data Input IQ signal.
     * @return Vector of instantaneous frequency values (Hz). Length = N-1.
     */
    static std::vector<float> instantaneousFrequency(const core::SignalData& data);
};

} // namespace features
} // namespace module2
