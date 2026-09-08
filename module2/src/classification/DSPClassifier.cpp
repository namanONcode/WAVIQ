#include "classification/DSPClassifier.hpp"

namespace module2 {
namespace classification {

core::ModulationResult DSPClassifier::classify(const core::SignalFeatures& features) {
    core::ModulationResult result;
    result.isMLBased = false;

    // A simple rule-based decision tree
    // Note: These thresholds are illustrative and should be fine-tuned against real datasets.
    const float ampVarThreshold = 0.05f;
    const float freqVarThreshold = 0.1f;

    // Amplitude variance helps distinguish between constant envelope (FSK/PSK) and non-constant (QAM/AM)
    if (features.amplitudeVar < ampVarThreshold) {
        // Constant envelope
        // Frequency variance helps separate FSK (high var) from PSK (low var)
        if (features.freqVar > freqVarThreshold) {
            result.modulationType = "FSK";
            result.confidence = 0.75f;
        } else {
            result.modulationType = "PSK";
            result.confidence = 0.75f;
        }
    } else {
        // Non-constant envelope (Amplitude variations present)
        result.modulationType = "QAM";
        result.confidence = 0.70f;
    }

    // In a real implementation, we would use higher order cumulants (C40, C42) to distinguish
    // between BPSK, QPSK, and 16-QAM more confidently.

    return result;
}

} // namespace classification
} // namespace module2
