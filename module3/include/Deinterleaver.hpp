#pragma once

#include <memory>
#include <vector>
#include "core/BitStream.hpp"

namespace module3 {

class Deinterleaver {
public:
    virtual ~Deinterleaver() = default;

    /**
     * @brief De-interleaves the input bitstream (soft or hard).
     * @param input The interleaved bitstream from Module 2.
     * @return A new BitStream containing the de-interleaved bits.
     */
    virtual std::shared_ptr<module2::core::BitStream> deinterleave(const std::shared_ptr<module2::core::BitStream>& input) = 0;
};

class BlockDeinterleaver : public Deinterleaver {
public:
    BlockDeinterleaver(size_t rows, size_t columns) : rows_(rows), cols_(columns) {}

    std::shared_ptr<module2::core::BitStream> deinterleave(const std::shared_ptr<module2::core::BitStream>& input) override;

private:
    size_t rows_;
    size_t cols_;
};

} // namespace module3
