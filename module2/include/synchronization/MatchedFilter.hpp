#pragma once
#include <vector>
#include <complex>

namespace module2 {
namespace synchronization {

class MatchedFilter {
public:
    /**
     * @brief Applies Root Raised Cosine (RRC) filtering before timing recovery.
     * @param input The input signal.
     * @return Filtered signal.
     */
    static std::vector<std::complex<float>> apply(const std::vector<std::complex<float>>& input);
};

} // namespace synchronization
} // namespace module2
