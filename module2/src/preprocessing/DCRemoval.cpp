#include "preprocessing/DCRemoval.hpp"

namespace module2 {
namespace preprocessing {

void DCRemoval::process(core::SignalData& data) {
    if (data.samples.empty()) {
        return;
    }

    double sumReal = 0.0;
    double sumImag = 0.0;

    for (const auto& sample : data.samples) {
        sumReal += sample.real();
        sumImag += sample.imag();
    }

    float meanReal = static_cast<float>(sumReal / data.samples.size());
    float meanImag = static_cast<float>(sumImag / data.samples.size());
    std::complex<float> mean(meanReal, meanImag);

    for (auto& sample : data.samples) {
        sample -= mean;
    }
}

} // namespace preprocessing
} // namespace module2
