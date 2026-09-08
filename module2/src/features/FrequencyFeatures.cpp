#include "features/FrequencyFeatures.hpp"
#include "features/PhaseFeatures.hpp"
#include <cmath>
#include <numeric>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace module2 {
namespace features {

std::vector<float> FrequencyFeatures::instantaneousFrequency(const core::SignalData& data) {
    if (data.samples.size() < 2) return {};

    // Compute unwrapped phase
    std::vector<float> wrappedPhase = PhaseFeatures::instantaneousPhase(data);
    std::vector<float> unwrapped = PhaseFeatures::unwrapPhase(wrappedPhase);

    const size_t M = unwrapped.size() - 1;
    std::vector<float> instFreq(M);

    // f[n] = (phi[n+1] - phi[n]) * Fs / (2*pi)
    float scale = static_cast<float>(data.sampleRate / (2.0 * M_PI));
    for (size_t i = 0; i < M; ++i) {
        instFreq[i] = (unwrapped[i + 1] - unwrapped[i]) * scale;
    }

    return instFreq;
}

FrequencyStats FrequencyFeatures::compute(const core::SignalData& data) {
    FrequencyStats stats = {};

    std::vector<float> instFreq = instantaneousFrequency(data);
    if (instFreq.empty()) return stats;

    const size_t M = instFreq.size();

    // Mean
    double sum = 0.0;
    for (size_t i = 0; i < M; ++i) sum += instFreq[i];
    stats.mean = static_cast<float>(sum / M);

    // Variance
    double varSum = 0.0;
    for (size_t i = 0; i < M; ++i) {
        double diff = instFreq[i] - stats.mean;
        varSum += diff * diff;
    }
    stats.variance = static_cast<float>(varSum / M);
    stats.stddev = std::sqrt(stats.variance);

    // Skewness and Kurtosis
    if (stats.stddev > 1e-12f) {
        double m3 = 0.0, m4 = 0.0;
        for (size_t i = 0; i < M; ++i) {
            double z = (instFreq[i] - stats.mean) / stats.stddev;
            double z2 = z * z;
            m3 += z2 * z;
            m4 += z2 * z2;
        }
        stats.skewness = static_cast<float>(m3 / M);
        stats.kurtosis = static_cast<float>(m4 / M - 3.0);
    } else {
        stats.skewness = 0.0f;
        stats.kurtosis = 0.0f;
    }

    return stats;
}

} // namespace features
} // namespace module2
