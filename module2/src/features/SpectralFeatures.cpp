#include "features/SpectralFeatures.hpp"
#include <cmath>
#include <numeric>
#include <algorithm>

namespace module2 {
namespace features {

SpectralStats SpectralFeatures::compute(const std::vector<float>& psd, const std::vector<float>& freqBins) {
    SpectralStats stats = {};

    if (psd.empty() || freqBins.empty() || psd.size() != freqBins.size()) return stats;

    const size_t N = psd.size();

    // ── Spectral Centroid ──
    // centroid = sum(f[k] * psd[k]) / sum(psd[k])
    double weightedSum = 0.0;
    double totalPower = 0.0;
    for (size_t k = 0; k < N; ++k) {
        weightedSum += static_cast<double>(freqBins[k]) * psd[k];
        totalPower += psd[k];
    }
    if (totalPower > 1e-30) {
        stats.centroid = static_cast<float>(weightedSum / totalPower);
    }

    // ── Spectral Spread ──
    // spread = sqrt( sum((f[k] - centroid)^2 * psd[k]) / sum(psd[k]) )
    double spreadSum = 0.0;
    for (size_t k = 0; k < N; ++k) {
        double diff = freqBins[k] - stats.centroid;
        spreadSum += diff * diff * psd[k];
    }
    if (totalPower > 1e-30) {
        stats.spread = static_cast<float>(std::sqrt(spreadSum / totalPower));
    }

    // ── Spectral Flatness ──
    // flatness = geometric_mean(PSD) / arithmetic_mean(PSD)
    // Computed in log domain to avoid overflow: exp(mean(log(PSD))) / mean(PSD)
    double arithmeticMean = totalPower / N;
    double logSum = 0.0;
    size_t validCount = 0;
    for (size_t k = 0; k < N; ++k) {
        if (psd[k] > 1e-30f) {
            logSum += std::log(static_cast<double>(psd[k]));
            validCount++;
        } else {
            logSum += std::log(1e-30); // Floor to avoid -inf
            validCount++;
        }
    }
    if (arithmeticMean > 1e-30 && validCount > 0) {
        double geometricMean = std::exp(logSum / validCount);
        stats.flatness = static_cast<float>(geometricMean / arithmeticMean);
    }

    // ── Spectral Rolloff (85%) ──
    // Frequency below which 85% of spectral energy resides
    double threshold = totalPower * 0.85;
    double cumulative = 0.0;
    stats.rolloff = freqBins.back(); // fallback
    for (size_t k = 0; k < N; ++k) {
        cumulative += psd[k];
        if (cumulative >= threshold) {
            stats.rolloff = freqBins[k];
            break;
        }
    }

    // ── Spectral Entropy ──
    // H = -sum(p[k] * log2(p[k])) / log2(N), normalized to [0, 1]
    if (totalPower > 1e-30) {
        double entropy = 0.0;
        for (size_t k = 0; k < N; ++k) {
            double p = psd[k] / totalPower;
            if (p > 1e-30) {
                entropy -= p * std::log2(p);
            }
        }
        double maxEntropy = std::log2(static_cast<double>(N));
        stats.entropy = (maxEntropy > 0.0) ? static_cast<float>(entropy / maxEntropy) : 0.0f;
    }

    // ── Peak to Mean Ratio ──
    float peakPsd = *std::max_element(psd.begin(), psd.end());
    stats.peakToMean = (arithmeticMean > 1e-30) ? static_cast<float>(peakPsd / arithmeticMean) : 0.0f;

    return stats;
}

} // namespace features
} // namespace module2
