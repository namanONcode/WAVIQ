#pragma once
#include "core/SignalData.hpp"
#include "core/AnalysisResult.hpp"

namespace module2 {
namespace features {

class FeatureExtractor {
public:
    /**
     * @brief Extracts all statistical, spectral, and cumulant features.
     * @param data The input signal data.
     * @return A populated SignalFeatures structure.
     */
    static core::SignalFeatures extractAll(const core::SignalData& data);
};

} // namespace features
} // namespace module2
