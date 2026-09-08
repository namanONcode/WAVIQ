#include "preprocessing/Filter.hpp"

namespace module2 {
namespace preprocessing {

void Filter::applyFIR(core::SignalData& data, const std::vector<float>& taps) {
    if (data.samples.empty() || taps.empty()) {
        return;
    }

    size_t numSamples = data.samples.size();
    size_t numTaps = taps.size();
    std::vector<std::complex<float>> output(numSamples, std::complex<float>(0.0f, 0.0f));

    for (size_t n = 0; n < numSamples; ++n) {
        for (size_t k = 0; k < numTaps; ++k) {
            if (n >= k) {
                output[n] += data.samples[n - k] * taps[k];
            }
        }
    }

    data.samples = std::move(output);
}

} // namespace preprocessing
} // namespace module2
