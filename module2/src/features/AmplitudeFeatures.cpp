#include "features/AmplitudeFeatures.hpp"
#include <cmath>
#include <numeric>
#include <algorithm>

namespace module2 {
namespace features {

std::vector<float> AmplitudeFeatures::envelope(const core::SignalData& data) {
    std::vector<float> env(data.samples.size());
    for (size_t i = 0; i < data.samples.size(); ++i) {
        env[i] = std::abs(data.samples[i]);
    }
    return env;
}

AmplitudeStats AmplitudeFeatures::compute(const core::SignalData& data) {
    AmplitudeStats stats = {};

    if (data.samples.empty()) return stats;

    const size_t N = data.samples.size();
    std::vector<float> env = envelope(data);

    // Mean
    double sum = 0.0;
    for (size_t i = 0; i < N; ++i) sum += env[i];
    stats.mean = static_cast<float>(sum / N);

    // Variance, RMS, Peak
    double varSum = 0.0;
    double sqSum = 0.0;
    stats.peak = 0.0f;
    for (size_t i = 0; i < N; ++i) {
        float diff = env[i] - stats.mean;
        varSum += static_cast<double>(diff) * diff;
        sqSum += static_cast<double>(env[i]) * env[i];
        if (env[i] > stats.peak) stats.peak = env[i];
    }
    stats.variance = static_cast<float>(varSum / N);
    stats.stddev = std::sqrt(stats.variance);
    stats.rms = static_cast<float>(std::sqrt(sqSum / N));

    // Crest factor
    stats.crestFactor = (stats.rms > 0.0f) ? (stats.peak / stats.rms) : 0.0f;

    // Skewness and Kurtosis
    if (stats.stddev > 1e-12f) {
        double m3 = 0.0, m4 = 0.0;
        for (size_t i = 0; i < N; ++i) {
            double z = (env[i] - stats.mean) / stats.stddev;
            double z2 = z * z;
            m3 += z2 * z;
            m4 += z2 * z2;
        }
        stats.skewness = static_cast<float>(m3 / N);
        stats.kurtosis = static_cast<float>(m4 / N - 3.0); // Excess kurtosis
    } else {
        stats.skewness = 0.0f;
        stats.kurtosis = 0.0f;
    }

    return stats;
}

} // namespace features
} // namespace module2
