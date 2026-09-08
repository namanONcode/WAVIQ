#pragma once
#include <vector>
#include <cstdint>

namespace module2 {
namespace core {

enum class BitStreamType {
    HARD,
    SOFT_FLOAT,
    SOFT_INT8
};

/**
 * @brief Base class for demodulated bits.
 * Supports both hard decisions (0/1) and soft decisions (LLRs) for FEC in Module 3.
 */
class BitStream {
public:
    virtual ~BitStream() = default;
    virtual BitStreamType getType() const = 0;
    virtual size_t size() const = 0;
};

class HardBitStream : public BitStream {
public:
    std::vector<uint8_t> bits;
    
    BitStreamType getType() const override { return BitStreamType::HARD; }
    size_t size() const override { return bits.size(); }
};

class SoftBitStreamFloat : public BitStream {
public:
    std::vector<float> llrs; // Log-Likelihood Ratios
    
    BitStreamType getType() const override { return BitStreamType::SOFT_FLOAT; }
    size_t size() const override { return llrs.size(); }
};

class SoftBitStreamInt8 : public BitStream {
public:
    std::vector<int8_t> llrs;
    
    BitStreamType getType() const override { return BitStreamType::SOFT_INT8; }
    size_t size() const override { return llrs.size(); }
};

} // namespace core
} // namespace module2
