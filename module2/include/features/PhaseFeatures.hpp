#pragma once
#include "core/SignalData.hpp"
#include <vector>

namespace module2 {
namespace features {

/**
 * @brief Computes phase-domain statistical features from IQ samples.
 *
 * Phase is computed as: phi[n] = atan2(Q[n], I[n]).
 * Phase is unwrapped before computing variance/higher-order stats to avoid
 * discontinuities at ±π corrupting the statistics.
 */
struct PhaseStats {
    float mean;         ///< Mean phase (radians, wrapped to [-π, π])
    float variance;     ///< Variance of unwrapped phase
    float stddev;       ///< Standard deviation
    float skewness;     ///< 3rd standardized moment
    float kurtosis;     ///< Excess kurtosis
};

class PhaseFeatures {
public:
    /**
     * @brief Extracts phase statistics from the signal.
     * @param data Input IQ signal.
     * @return PhaseStats populated with computed features.
     */
    static PhaseStats compute(const core::SignalData& data);

    /**
     * @brief Computes the instantaneous phase phi[n] = atan2(Q, I).
     * @param data Input IQ signal.
     * @return Vector of wrapped phase values in [-π, π].
     */
    static std::vector<float> instantaneousPhase(const core::SignalData& data);

    /**
     * @brief Unwraps a phase vector to remove 2π discontinuities.
     * @param phase Input wrapped phase vector.
     * @return Unwrapped continuous phase vector.
     */
    static std::vector<float> unwrapPhase(const std::vector<float>& phase);
};

} // namespace features
} // namespace module2
