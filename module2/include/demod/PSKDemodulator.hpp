#pragma once
#include "demod/Demodulator.hpp"

namespace module2 {
namespace demod {

class PSKDemodulator : public Demodulator {
public:
    enum class Mode {
        BPSK,
        QPSK,
        PSK8
    };

    explicit PSKDemodulator(Mode mode = Mode::BPSK) : mode_(mode) {}

    std::shared_ptr<core::SoftBitStreamFloat> demodulate(const std::vector<std::complex<float>>& symbols) override;

private:
    Mode mode_;
};

} // namespace demod
} // namespace module2
