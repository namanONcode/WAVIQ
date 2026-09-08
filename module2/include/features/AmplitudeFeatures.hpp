#pragma once
#include "core/SignalData.hpp"
#include <vector>

namespace module2 {
namespace features {

/**
 * @brief Computes amplitude-domain statistical features from IQ samples.
 *
 * All features are computed from the instantaneous amplitude envelope: a[n] = |x[n]|.
 */
struct AmplitudeStats {
    float mean;         ///< Mean amplitude: E[a]
    float variance;     ///< Variance: E[(a - mean)^2]
    float stddev;       ///< Standard deviation: sqrt(variance)
    float skewness;     ///< 3rd-order standardized moment (asymmetry)
    float kurtosis;     ///< 4th-order standardized moment (peakedness), excess kurtosis
    float peak;         ///< Maximum amplitude
    float rms;          ///< Root-mean-square amplitude
    float crestFactor;  ///< peak / rms (peak-to-average ratio)
};

class AmplitudeFeatures {
public:
    /**
     * @brief Extracts amplitude statistics from the signal.
     * @param data Input IQ signal.
     * @return AmplitudeStats populated with computed features.
     */
    static AmplitudeStats compute(const core::SignalData& data);

    /**
     * @brief Computes the instantaneous amplitude envelope |x[n]|.
     * @param data Input IQ signal.
     * @return Vector of amplitude values.
     */
    static std::vector<float> envelope(const core::SignalData& data);
};

} // namespace features
} // namespace module2
