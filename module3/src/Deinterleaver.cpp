#include "Deinterleaver.hpp"
#include "Module3Exceptions.hpp"

namespace module3 {

std::shared_ptr<module2::core::BitStream> BlockDeinterleaver::deinterleave(const std::shared_ptr<module2::core::BitStream>& input) {
    if (!input) {
        throw DeinterleaverError("Input bitstream is null.");
    }
    
    // De-interleaving is simply reading column-wise and writing row-wise (or vice-versa depending on the convention)
    // Here we assume data was interleaved by writing rows and reading columns,
    // so to de-interleave, we write columns and read rows.
    
    size_t totalSize = input->size();
    if (totalSize != rows_ * cols_) {
        throw DeinterleaverError("Input size (" + std::to_string(totalSize) + 
                                 ") does not match rows * columns (" + std::to_string(rows_ * cols_) + ").");
    }

    if (input->getType() == module2::core::BitStreamType::HARD) {
        auto hardIn = std::static_pointer_cast<module2::core::HardBitStream>(input);
        auto hardOut = std::make_shared<module2::core::HardBitStream>();
        hardOut->bits.resize(totalSize);
        
        for (size_t r = 0; r < rows_; ++r) {
            for (size_t c = 0; c < cols_; ++c) {
                hardOut->bits[r * cols_ + c] = hardIn->bits[c * rows_ + r];
            }
        }
        return hardOut;
    } 
    else if (input->getType() == module2::core::BitStreamType::SOFT_FLOAT) {
        auto softIn = std::static_pointer_cast<module2::core::SoftBitStreamFloat>(input);
        auto softOut = std::make_shared<module2::core::SoftBitStreamFloat>();
        softOut->llrs.resize(totalSize);
        
        for (size_t r = 0; r < rows_; ++r) {
            for (size_t c = 0; c < cols_; ++c) {
                softOut->llrs[r * cols_ + c] = softIn->llrs[c * rows_ + r];
            }
        }
        return softOut;
    }
    else {
        throw DeinterleaverError("Unsupported BitStreamType encountered.");
    }
}

} // namespace module3
