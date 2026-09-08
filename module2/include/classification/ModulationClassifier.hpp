#pragma once
#include "core/AnalysisResult.hpp"

namespace module2 {
namespace classification {

class ModulationClassifier {
public:
    /**
     * @brief Hybrid classifier that can use DSP rules or ML.
     * Currently relies on DSPClassifier as per M7.
     * @param features The extracted signal features.
     * @return ModulationResult with final decision.
     */
    static core::ModulationResult classify(const core::SignalFeatures& features);
};

} // namespace classification
} // namespace module2
