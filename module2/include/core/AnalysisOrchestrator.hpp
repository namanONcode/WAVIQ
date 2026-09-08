#pragma once
#include "core/SignalData.hpp"
#include "core/AnalysisResult.hpp"

namespace module2 {
namespace core {

class AnalysisOrchestrator {
public:
    /**
     * @brief Processes the input signal through the entire Module 2 pipeline.
     * Integrates Preprocessing, Analysis, Feature Extraction, Classification,
     * Synchronization, and Demodulation.
     * 
     * @param data The input baseband signal.
     * @return The comprehensive analysis result containing parameters, features, and soft bits.
     */
    static AnalysisResult analyze(SignalData data);
};

} // namespace core
} // namespace module2
