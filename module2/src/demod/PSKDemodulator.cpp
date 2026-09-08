#include "demod/PSKDemodulator.hpp"

namespace module2 {
namespace demod {

std::shared_ptr<core::SoftBitStreamFloat> PSKDemodulator::demodulate(const std::vector<std::complex<float>>& symbols) {
    auto stream = std::make_shared<core::SoftBitStreamFloat>();
    
    // Placeholder LLR calculation for BPSK
    // LLR is approximated by the real part of the received symbol
    if (mode_ == Mode::BPSK) {
        for (const auto& sym : symbols) {
            stream->llrs.push_back(sym.real()); // simple soft bit approximation
        }
    } else if (mode_ == Mode::QPSK) {
        for (const auto& sym : symbols) {
            stream->llrs.push_back(sym.real());
            stream->llrs.push_back(sym.imag());
        }
    }
    
    return stream;
}

} // namespace demod
} // namespace module2
