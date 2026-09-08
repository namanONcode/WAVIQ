#include "analysis/SpectrumAnalyzer.hpp"
#include <algorithm>
#include <numeric>

namespace module2 {
namespace analysis {

float SpectrumAnalyzer::computePeakFrequency(const std::vector<float>& psd, const std::vector<float>& freqBins) {
    if (psd.empty() || freqBins.empty() || psd.size() != freqBins.size()) {
        return 0.0f;
    }

    auto maxIt = std::max_element(psd.begin(), psd.end());
    size_t maxIdx = std::distance(psd.begin(), maxIt);

    return freqBins[maxIdx];
}

float SpectrumAnalyzer::computeSpectralCentroid(const std::vector<float>& psd, const std::vector<float>& freqBins) {
    if (psd.empty() || freqBins.empty() || psd.size() != freqBins.size()) {
        return 0.0f;
    }

    float numerator = 0.0f;
    float denominator = 0.0f;

    for (size_t i = 0; i < psd.size(); ++i) {
        numerator += psd[i] * freqBins[i];
        denominator += psd[i];
    }

    if (denominator == 0.0f) {
        return 0.0f;
    }

    return numerator / denominator;
}

float SpectrumAnalyzer::estimateNoiseFloor(const std::vector<float>& psd) {
    if (psd.empty()) {
        return 0.0f;
    }

    // A simple heuristic for noise floor: sort the PSD and take the median
    // or average of the lowest quartile. Here we use the median of the lower 25%.
    std::vector<float> sortedPsd = psd;
    std::sort(sortedPsd.begin(), sortedPsd.end());

    size_t quartileIdx = sortedPsd.size() / 4;
    if (quartileIdx == 0) {
        return sortedPsd[0]; // Fallback to min
    }

    float sum = 0.0f;
    for (size_t i = 0; i < quartileIdx; ++i) {
        sum += sortedPsd[i];
    }

    return sum / quartileIdx;
}

} // namespace analysis
} // namespace module2
