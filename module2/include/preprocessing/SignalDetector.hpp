#pragma once
#include "core/SignalData.hpp"

namespace module2 {
namespace preprocessing {

class SignalDetector {
public:
    /**
     * @brief Extracts the active region of a signal by removing noise tails using energy thresholding.
     * 
     * @param data The signal data to process. It will be modified in-place.
     * @param energyThreshold The threshold factor relative to the average noise floor.
     * @return true if a signal region was found, false otherwise.
     */
    static bool extractActiveRegion(core::SignalData& data, float energyThreshold = 3.0f);
};

} // namespace preprocessing
} // namespace module2
