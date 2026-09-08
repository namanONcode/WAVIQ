#pragma once
#include "demod/Demodulator.hpp"

namespace module2 {
namespace demod {

class QAMDemodulator : public Demodulator {
public:
    enum class Order {
        QAM16,
        QAM64
    };

    explicit QAMDemodulator(Order order = Order::QAM16) : order_(order) {}

    std::shared_ptr<core::SoftBitStreamFloat> demodulate(const std::vector<std::complex<float>>& symbols) override;

private:
    Order order_;
};

} // namespace demod
} // namespace module2
