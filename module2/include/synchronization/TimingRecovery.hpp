#pragma once
#include <vector>
#include <complex>

namespace module2 {
namespace synchronization {

class TimingRecovery {
public:
    /**
     * @brief Implements Gardner Timing Error Detector (TED) to recover symbols.
     * @param input The input signal (after matched filtering).
     * @param samplesPerSymbol The oversampling factor.
     * @return Synchronized symbols (1 sample per symbol).
     */
    static std::vector<std::complex<float>> recover(const std::vector<std::complex<float>>& input, int samplesPerSymbol);
};

} // namespace synchronization
} // namespace module2
