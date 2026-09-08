#include "classification/ModulationClassifier.hpp"
#include "classification/DSPClassifier.hpp"

namespace module2 {
namespace classification {

core::ModulationResult ModulationClassifier::classify(const core::SignalFeatures& features) {
    // For M7, we rely entirely on the rule-based DSP Classifier.
    // In the future, this engine can evaluate MLClassifier results and pick the one with highest confidence.
    return DSPClassifier::classify(features);
}

} // namespace classification
} // namespace module2
