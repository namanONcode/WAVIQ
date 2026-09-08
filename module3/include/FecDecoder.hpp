#pragma once

#include <memory>
#include <vector>
#include <string>
#include "core/BitStream.hpp"

namespace module3 {

struct FecResult {
    bool success;
    float bitErrorRate;
    std::shared_ptr<module2::core::HardBitStream> decodedBits;
};

class FecDecoder {
public:
    virtual ~FecDecoder() = default;

    /**
     * @brief Decodes the bitstream, preferably utilizing soft decisions if available.
     * @param input The de-interleaved bitstream.
     * @return FecResult containing the hard-decoded bits and error metrics.
     */
    virtual FecResult decode(const std::shared_ptr<module2::core::BitStream>& input) = 0;
};

class ViterbiDecoder : public FecDecoder {
public:
    ViterbiDecoder(int constraintLength, const std::vector<int>& polynomials) 
        : k_(constraintLength), polys_(polynomials) {}

    FecResult decode(const std::shared_ptr<module2::core::BitStream>& input) override;

private:
    int k_;
    std::vector<int> polys_;
};

class RSDecoder : public FecDecoder {
public:
    RSDecoder(int symbolSize, int blockLength, int messageLength)
        : m_(symbolSize), n_(blockLength), k_(messageLength) {}

    FecResult decode(const std::shared_ptr<module2::core::BitStream>& input) override;

private:
    int m_;
    int n_;
    int k_;
};

class ConcatenatedDecoder : public FecDecoder {
public:
    ConcatenatedDecoder(std::shared_ptr<FecDecoder> inner, std::shared_ptr<FecDecoder> outer)
        : inner_(inner), outer_(outer) {}

    FecResult decode(const std::shared_ptr<module2::core::BitStream>& input) override;

private:
    std::shared_ptr<FecDecoder> inner_;
    std::shared_ptr<FecDecoder> outer_;
};

class LDPCDecoder : public FecDecoder {
public:
    // Requires parity-check matrix (AList format) path or dimensions
    LDPCDecoder(const std::string& alistFile) : alistFile_(alistFile) {}

    FecResult decode(const std::shared_ptr<module2::core::BitStream>& input) override;

private:
    std::string alistFile_;
};

} // namespace module3
