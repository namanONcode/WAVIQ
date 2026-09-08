#include "FecDecoder.hpp"
#include "Module3Exceptions.hpp"
#include <iostream>

namespace module3 {

// --- Viterbi Decoder (Skeleton) ---
FecResult ViterbiDecoder::decode(const std::shared_ptr<module2::core::BitStream>& input) {
    if (!input) {
        throw FecDecoderError("Input bitstream is null.");
    }

    // In a real implementation, we would pass 'input' (soft or hard bits) 
    // to libfec's Viterbi decoder here.
    // For this MVP skeleton, we simply threshold soft bits to hard bits and pass them through,
    // simulating a rate 1/2 decoder by just returning half the bits (dummy behavior).
    
    FecResult result;
    result.success = true;
    result.bitErrorRate = 0.0f;
    result.decodedBits = std::make_shared<module2::core::HardBitStream>();
    
    if (input->getType() == module2::core::BitStreamType::SOFT_FLOAT) {
        auto soft = std::static_pointer_cast<module2::core::SoftBitStreamFloat>(input);
        for (size_t i = 0; i < soft->llrs.size(); i += 2) { // Simulating rate 1/2
            result.decodedBits->bits.push_back(soft->llrs[i] > 0 ? 1 : 0);
        }
    } else if (input->getType() == module2::core::BitStreamType::HARD) {
        auto hard = std::static_pointer_cast<module2::core::HardBitStream>(input);
        for (size_t i = 0; i < hard->bits.size(); i += 2) { // Simulating rate 1/2
            result.decodedBits->bits.push_back(hard->bits[i]);
        }
    }

    return result;
}

// --- Reed-Solomon Decoder (Skeleton) ---
FecResult RSDecoder::decode(const std::shared_ptr<module2::core::BitStream>& input) {
    if (!input) {
        throw FecDecoderError("Input bitstream is null.");
    }
    
    // RS operates on block boundaries.
    // For this MVP skeleton, we simply pass the bits through.
    
    FecResult result;
    result.success = true;
    result.bitErrorRate = 0.0f;
    result.decodedBits = std::make_shared<module2::core::HardBitStream>();

    if (input->getType() == module2::core::BitStreamType::HARD) {
        auto hard = std::static_pointer_cast<module2::core::HardBitStream>(input);
        result.decodedBits->bits = hard->bits; // Pass-through
    } else {
        // RS generally takes hard bits. If we get soft bits, we should threshold them first.
        auto soft = std::static_pointer_cast<module2::core::SoftBitStreamFloat>(input);
        for (float llr : soft->llrs) {
            result.decodedBits->bits.push_back(llr > 0 ? 1 : 0);
        }
    }

    return result;
}

// --- Concatenated Decoder (Skeleton) ---
FecResult ConcatenatedDecoder::decode(const std::shared_ptr<module2::core::BitStream>& input) {
    if (!input) {
        throw FecDecoderError("Input bitstream is null.");
    }
    
    if (!inner_ || !outer_) {
        throw FecDecoderError("Inner or outer decoder is not initialized.");
    }

    FecResult result;
    
    // 1. Inner Decode (Usually Viterbi)
    auto innerResult = inner_->decode(input);
    if (!innerResult.success) {
        result.success = false;
        return result;
    }

    // 2. Outer Decode (Usually RS)
    // The inner decoder outputs hard bits, which we feed to the outer decoder.
    auto outerResult = outer_->decode(innerResult.decodedBits);
    
    // Combine metrics
    result.success = outerResult.success;
    result.bitErrorRate = (innerResult.bitErrorRate + outerResult.bitErrorRate) / 2.0f;
    result.decodedBits = outerResult.decodedBits;

    return result;
}

// --- LDPC Decoder (Skeleton) ---
FecResult LDPCDecoder::decode(const std::shared_ptr<module2::core::BitStream>& input) {
    if (!input) {
        throw FecDecoderError("Input bitstream is null.");
    }

    // For this MVP skeleton, we threshold the soft bits.
    // In production, this uses aff3ct's LDPC module with the provided AList matrix.
    
    FecResult result;
    result.success = true;
    result.bitErrorRate = 0.0f;
    result.decodedBits = std::make_shared<module2::core::HardBitStream>();

    if (input->getType() == module2::core::BitStreamType::SOFT_FLOAT) {
        auto soft = std::static_pointer_cast<module2::core::SoftBitStreamFloat>(input);
        for (float llr : soft->llrs) {
            result.decodedBits->bits.push_back(llr > 0 ? 1 : 0);
        }
    } else if (input->getType() == module2::core::BitStreamType::HARD) {
        auto hard = std::static_pointer_cast<module2::core::HardBitStream>(input);
        result.decodedBits->bits = hard->bits;
    }

    return result;
}

} // namespace module3
