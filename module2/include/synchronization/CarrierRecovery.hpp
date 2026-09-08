#pragma once
#include <vector>
#include <complex>

namespace module2 {
namespace synchronization {

class CarrierRecovery {
public:
    /**
     * @brief Implements a Costas Loop or Phase-Locked Loop (PLL) to correct carrier offsets.
     * @param input The input signal with frequency/phase offset.
     * @return Synchronized samples.
     */
    static std::vector<std::complex<float>> recover(const std::vector<std::complex<float>>& input);
};

} // namespace synchronization
} // namespace module2
