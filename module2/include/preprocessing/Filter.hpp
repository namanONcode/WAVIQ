#pragma once
#include "core/SignalData.hpp"
#include <vector>

namespace module2 {
namespace preprocessing {

class Filter {
public:
    /**
     * @brief Applies a Finite Impulse Response (FIR) filter to the signal.
     * 
     * @param data The signal data to process. It will be modified in-place.
     * @param taps The FIR filter coefficients.
     */
    static void applyFIR(core::SignalData& data, const std::vector<float>& taps);
};

} // namespace preprocessing
} // namespace module2
