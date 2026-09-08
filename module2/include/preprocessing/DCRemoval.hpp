#pragma once
#include "core/SignalData.hpp"

namespace module2 {
namespace preprocessing {

class DCRemoval {
public:
    /**
     * @brief Removes the DC offset (mean) from the I and Q components of the signal.
     * 
     * @param data The signal data to process. It will be modified in-place.
     */
    static void process(core::SignalData& data);
};

} // namespace preprocessing
} // namespace module2
