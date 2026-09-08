#include "features/PhaseFeatures.hpp"
#include <cmath>
#include <numeric>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace module2 {
namespace features {

std::vector<float> PhaseFeatures::instantaneousPhase(const core::SignalData& data) {
    std::vector<float> phase(data.samples.size());
    for (size_t i = 0; i < data.samples.size(); ++i) {
        phase[i] = std::atan2(data.samples[i].imag(), data.samples[i].real());
    }
    return phase;
}

std::vector<float> PhaseFeatures::unwrapPhase(const std::vector<float>& phase) {
    if (phase.empty()) return {};

    std::vector<float> unwrapped(phase.size());
    unwrapped[0] = phase[0];

    for (size_t i = 1; i < phase.size(); ++i) {
        float diff = phase[i] - phase[i - 1];
        // Wrap diff to [-π, π]
        while (diff > static_cast<float>(M_PI))  diff -= static_cast<float>(2.0 * M_PI);
        while (diff < static_cast<float>(-M_PI)) diff += static_cast<float>(2.0 * M_PI);
        unwrapped[i] = unwrapped[i - 1] + diff;
    }

    return unwrapped;
}

PhaseStats PhaseFeatures::compute(const core::SignalData& data) {
    PhaseStats stats = {};

    if (data.samples.size() < 2) return stats;

    const size_t N = data.samples.size();

    // Compute wrapped phase
    std::vector<float> wrappedPhase = instantaneousPhase(data);

    // Compute mean of wrapped phase using circular mean (atan2 of avg sin/cos)
    double sinSum = 0.0, cosSum = 0.0;
    for (size_t i = 0; i < N; ++i) {
        sinSum += std::sin(wrappedPhase[i]);
        cosSum += std::cos(wrappedPhase[i]);
    }
    stats.mean = static_cast<float>(std::atan2(sinSum / N, cosSum / N));

    // Unwrap phase for variance computation
    std::vector<float> unwrapped = unwrapPhase(wrappedPhase);

    // Remove linear trend (carrier) to compute deviation stats
    // Linear fit: phi_trend[n] = a + b*n
    // We use the detrended phase for variance/skewness/kurtosis
    double sumN = 0.0, sumPhi = 0.0, sumNPhi = 0.0, sumN2 = 0.0;
    for (size_t i = 0; i < N; ++i) {
        double n = static_cast<double>(i);
        sumN += n;
        sumPhi += unwrapped[i];
        sumNPhi += n * unwrapped[i];
        sumN2 += n * n;
    }
    double denom = N * sumN2 - sumN * sumN;
    double b = 0.0, a = 0.0;
    if (std::abs(denom) > 1e-15) {
        b = (N * sumNPhi - sumN * sumPhi) / denom;
        a = (sumPhi - b * sumN) / N;
    }

    // Detrend
    std::vector<float> detrended(N);
    for (size_t i = 0; i < N; ++i) {
        detrended[i] = unwrapped[i] - static_cast<float>(a + b * i);
    }

    // Variance of detrended phase
    double dtMean = 0.0;
    for (size_t i = 0; i < N; ++i) dtMean += detrended[i];
    dtMean /= N;

    double varSum = 0.0;
    for (size_t i = 0; i < N; ++i) {
        double diff = detrended[i] - dtMean;
        varSum += diff * diff;
    }
    stats.variance = static_cast<float>(varSum / N);
    stats.stddev = std::sqrt(stats.variance);

    // Skewness and Kurtosis of detrended phase
    if (stats.stddev > 1e-12f) {
        double m3 = 0.0, m4 = 0.0;
        for (size_t i = 0; i < N; ++i) {
            double z = (detrended[i] - dtMean) / stats.stddev;
            double z2 = z * z;
            m3 += z2 * z;
            m4 += z2 * z2;
        }
        stats.skewness = static_cast<float>(m3 / N);
        stats.kurtosis = static_cast<float>(m4 / N - 3.0);
    } else {
        stats.skewness = 0.0f;
        stats.kurtosis = 0.0f;
    }

    return stats;
}

} // namespace features
} // namespace module2
