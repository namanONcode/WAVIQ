#include "preprocessing/Normalizer.hpp"
#include <cmath>
#include <algorithm>

namespace module2 {
namespace preprocessing {

void Normalizer::process(core::SignalData& data) {
    if (data.samples.empty()) {
        return;
    }

    float maxAmplitude = 0.0f;
    for (const auto& sample : data.samples) {
        float amplitude = std::abs(sample);
        if (amplitude > maxAmplitude) {
            maxAmplitude = amplitude;
        }
    }

    if (maxAmplitude > 0.0f) {
        for (auto& sample : data.samples) {
            sample /= maxAmplitude;
        }
    }
}

} // namespace preprocessing
} // namespace module2
