#pragma once
#include "demod/Demodulator.hpp"

namespace module2 {
namespace demod {

class FSKDemodulator : public Demodulator {
public:
    std::shared_ptr<core::SoftBitStreamFloat> demodulate(const std::vector<std::complex<float>>& symbols) override;
};

} // namespace demod
} // namespace module2
