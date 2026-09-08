#pragma once
#include "core/SignalData.hpp"
#include "core/BitStream.hpp"
#include <memory>
#include <vector>
#include <complex>

namespace module2 {
namespace demod {

class Demodulator {
public:
    virtual ~Demodulator() = default;
    
    /**
     * @brief Demodulate synchronized symbols and return LLRs (Soft decisions) for FEC in Module 3.
     * @param symbols The synchronized complex symbols.
     * @return A SoftBitStreamFloat containing Log-Likelihood Ratios (LLRs).
     */
    virtual std::shared_ptr<core::SoftBitStreamFloat> demodulate(const std::vector<std::complex<float>>& symbols) = 0;
};

} // namespace demod
} // namespace module2
