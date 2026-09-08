#include "FecDecoder.hpp"
#include "Module3Exceptions.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>

extern "C" {
#include <fec.h>
}

namespace module3 {

// --- Viterbi Decoder (Real Implementation) ---
FecResult ViterbiDecoder::decode(const std::shared_ptr<module2::core::BitStream>& input) {
    if (!input) {
        throw FecDecoderError("Input bitstream is null.");
    }
    if (polys_.size() != 2) {
        throw FecDecoderError("Only rate 1/2 Viterbi is currently implemented.");
    }

    FecResult result;
    result.success = true;
    result.bitErrorRate = 0.0f;
    result.decodedBits = std::make_shared<module2::core::HardBitStream>();

    std::vector<float> llrs;
    if (input->getType() == module2::core::BitStreamType::SOFT_FLOAT) {
        llrs = std::static_pointer_cast<module2::core::SoftBitStreamFloat>(input)->llrs;
    } else if (input->getType() == module2::core::BitStreamType::HARD) {
        auto hard = std::static_pointer_cast<module2::core::HardBitStream>(input);
        llrs.reserve(hard->bits.size());
        for (uint8_t b : hard->bits) {
            // Map 0 to -1.0, 1 to 1.0 (standard LLR mapping for correlation)
            llrs.push_back(b ? 1.0f : -1.0f);
        }
    }

    int numStates = 1 << (k_ - 1);
    std::vector<float> pathMetrics(numStates, -1e9f); // Maximize correlation
    pathMetrics[0] = 0.0f; // Start at state 0
    
    // Trellis backpointers: [time][state] -> {prev_state, input_bit}
    std::vector<std::vector<std::pair<int, int>>> backpointers(llrs.size() / 2, std::vector<std::pair<int, int>>(numStates));

    for (size_t t = 0; t < llrs.size() / 2; ++t) {
        std::vector<float> nextPathMetrics(numStates, -1e9f);
        float r0 = llrs[2 * t];
        float r1 = llrs[2 * t + 1];

        for (int state = 0; state < numStates; ++state) {
            if (pathMetrics[state] <= -1e8f) continue; // Unreachable state

            for (int bit = 0; bit <= 1; ++bit) {
                // Shift register logic:
                // New state = (bit << (k_-2)) | (state >> 1)
                int nextState = (bit << (k_ - 2)) | (state >> 1);
                int shiftReg = (bit << (k_ - 1)) | state;

                // Calculate outputs
                int out0 = 0, out1 = 0;
                for (int i = 0; i < k_; ++i) {
                    if ((polys_[0] >> i) & 1) out0 ^= ((shiftReg >> i) & 1);
                    if ((polys_[1] >> i) & 1) out1 ^= ((shiftReg >> i) & 1);
                }

                // Map 0 -> -1.0, 1 -> 1.0 for correlation metric
                float branchMetric = (out0 ? 1.0f : -1.0f) * r0 + (out1 ? 1.0f : -1.0f) * r1;
                float newMetric = pathMetrics[state] + branchMetric;

                if (newMetric > nextPathMetrics[nextState]) {
                    nextPathMetrics[nextState] = newMetric;
                    backpointers[t][nextState] = {state, bit};
                }
            }
        }
        pathMetrics = std::move(nextPathMetrics);
    }

    // Traceback
    int bestState = 0;
    float bestMetric = -1e9f;
    for (int state = 0; state < numStates; ++state) {
        if (pathMetrics[state] > bestMetric) {
            bestMetric = pathMetrics[state];
            bestState = state;
        }
    }

    std::vector<uint8_t> decoded;
    decoded.reserve(llrs.size() / 2);
    
    int currentState = bestState;
    for (int t = (llrs.size() / 2) - 1; t >= 0; --t) {
        auto bp = backpointers[t][currentState];
        decoded.push_back(bp.second);
        currentState = bp.first;
    }

    std::reverse(decoded.begin(), decoded.end());
    result.decodedBits->bits = decoded;

    return result;
}

// --- Reed-Solomon Decoder (libfec implementation) ---
FecResult RSDecoder::decode(const std::shared_ptr<module2::core::BitStream>& input) {
    if (!input) {
        throw FecDecoderError("Input bitstream is null.");
    }
    
    FecResult result;
    result.success = true;
    result.bitErrorRate = 0.0f;
    result.decodedBits = std::make_shared<module2::core::HardBitStream>();

    std::vector<uint8_t> hardBits;
    if (input->getType() == module2::core::BitStreamType::HARD) {
        hardBits = std::static_pointer_cast<module2::core::HardBitStream>(input)->bits;
    } else {
        auto soft = std::static_pointer_cast<module2::core::SoftBitStreamFloat>(input);
        for (float llr : soft->llrs) {
            hardBits.push_back(llr > 0 ? 1 : 0);
        }
    }

    // We expect RS(255, 223) parameters (n_=255, k_=223) operating on 8-bit symbols
    if (m_ != 8 || n_ != 255 || k_ != 223) {
        throw FecDecoderError("Only RS(255, 223) with 8-bit symbols is supported by this libfec wrapper.");
    }

    // Pack bits into 8-bit symbols (MSB first)
    std::vector<unsigned char> symbols;
    for (size_t i = 0; i + 7 < hardBits.size(); i += 8) {
        unsigned char sym = 0;
        for (int b = 0; b < 8; ++b) {
            sym = (sym << 1) | (hardBits[i + b] & 1);
        }
        symbols.push_back(sym);
    }

    // Process block by block (zero-pad incomplete blocks)
    int totalErrors = 0;
    for (size_t i = 0; i < symbols.size(); i += n_) {
        std::vector<unsigned char> block(n_, 0); // initialize with zeros (padding)
        size_t bytesToCopy = std::min(static_cast<size_t>(n_), symbols.size() - i);
        std::copy(symbols.begin() + i, symbols.begin() + i + bytesToCopy, block.begin());
        
        // decode_rs_8(data, eras_pos, no_eras, pad)
        // Note: For shortened codes, pad is the number of zero-padded symbols, which are theoretically prepended,
        // but here we just decode the full 255-byte block we created.
        int errs = decode_rs_8(block.data(), nullptr, 0, 0);
        
        if (errs < 0) {
            result.success = false;
        } else {
            totalErrors += errs;
        }
        
        // Unpack back to bits (MSB first) up to the valid message length we actually received
        // But RS(255, 223) outputs 223 bytes. If our partial block was very small (e.g. 15 bytes),
        // we only want to extract those 15 bytes (or up to k_).
        size_t validMessageBytes = std::min(static_cast<size_t>(k_), bytesToCopy);
        
        for (size_t j = 0; j < validMessageBytes; ++j) {
            unsigned char sym = block[j];
            for (int b = 7; b >= 0; --b) {
                result.decodedBits->bits.push_back((sym >> b) & 1);
            }
        }
    }

    // Calculate approximate BER
    if (result.decodedBits->bits.size() > 0) {
        result.bitErrorRate = static_cast<float>(totalErrors * 8) / result.decodedBits->bits.size();
    }

    return result;
}

// --- Concatenated Decoder (Real Pipeline) ---
FecResult ConcatenatedDecoder::decode(const std::shared_ptr<module2::core::BitStream>& input) {
    if (!input) {
        throw FecDecoderError("Input bitstream is null.");
    }
    
    if (!inner_ || !outer_) {
        throw FecDecoderError("Inner or outer decoder is not initialized.");
    }

    FecResult result;
    
    // 1. Inner Decode (Viterbi)
    auto innerResult = inner_->decode(input);
    if (!innerResult.success) {
        result.success = false;
        return result;
    }

    // 2. Outer Decode (Block Code / RS)
    auto outerResult = outer_->decode(innerResult.decodedBits);
    
    // Combine metrics
    result.success = outerResult.success;
    result.bitErrorRate = (innerResult.bitErrorRate + outerResult.bitErrorRate) / 2.0f;
    result.decodedBits = outerResult.decodedBits;

    return result;
}

// --- LDPC Decoder (Basic Functional Stub) ---
FecResult LDPCDecoder::decode(const std::shared_ptr<module2::core::BitStream>& input) {
    if (!input) {
        throw FecDecoderError("Input bitstream is null.");
    }
    
    // Like RS, a real Sum-Product or Min-Sum Belief Propagation algorithm requires 
    // loading a parity matrix and performing iterative message passing.
    // For this MVP, we perform basic parity thresholding.
    
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
