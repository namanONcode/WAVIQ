#pragma once
#include "core/AnalysisResult.hpp"

namespace module2 {
namespace classification {

class DSPClassifier {
public:
    /**
     * @brief Classifies modulation using a rule-based decision tree on extracted features.
     * @param features The extracted signal features.
     * @return ModulationResult containing predicted type and confidence.
     */
    static core::ModulationResult classify(const core::SignalFeatures& features);
};

} // namespace classification
} // namespace module2
