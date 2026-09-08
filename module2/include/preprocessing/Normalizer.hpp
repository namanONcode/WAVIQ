#pragma once
#include "core/SignalData.hpp"

namespace module2 {
namespace preprocessing {

class Normalizer {
public:
    /**
     * @brief Normalizes the amplitude of the signal so the maximum absolute value is 1.0.
     * 
     * @param data The signal data to process. It will be modified in-place.
     */
    static void process(core::SignalData& data);
};

} // namespace preprocessing
} // namespace module2
