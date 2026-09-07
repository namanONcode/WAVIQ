#pragma once

#include <complex>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "module1/SignalTypes.h"

namespace module1 {

// Strategy interface for decoding raw interleaved IQ byte streams into
// normalized std::complex<float> samples.
class IIqSampleDecoder {
public:
    virtual ~IIqSampleDecoder() = default;

    // Number of bytes required per complex sample (I + Q combined).
    virtual size_t bytes_per_sample() const = 0;

    // Decodes a raw byte buffer into normalized [-1.0, 1.0] complex float samples.
    // Throws UnsupportedFormatError if byte_count is not an exact multiple of bytes_per_sample().
    virtual std::vector<std::complex<float>> decode(
        const uint8_t* data,
        size_t byte_count,
        ByteOrder byte_order) const = 0;

    // Human-readable datatype name (e.g. "int8", "int16", "float32").
    virtual const std::string& datatype_name() const = 0;
};

class Int8IqDecoder : public IIqSampleDecoder {
public:
    size_t bytes_per_sample() const override { return 2; }
    std::vector<std::complex<float>> decode(
        const uint8_t* data,
        size_t byte_count,
        ByteOrder byte_order) const override;
    const std::string& datatype_name() const override;
};

class Int16IqDecoder : public IIqSampleDecoder {
public:
    size_t bytes_per_sample() const override { return 4; }
    std::vector<std::complex<float>> decode(
        const uint8_t* data,
        size_t byte_count,
        ByteOrder byte_order) const override;
    const std::string& datatype_name() const override;
};

class Float32IqDecoder : public IIqSampleDecoder {
public:
    size_t bytes_per_sample() const override { return 8; }
    std::vector<std::complex<float>> decode(
        const uint8_t* data,
        size_t byte_count,
        ByteOrder byte_order) const override;
    const std::string& datatype_name() const override;
};

class IqSampleDecoderFactory {
public:
    // Returns the appropriate decoder for the datatype ("int8", "int16", "float32").
    // Throws UnsupportedFormatError if the datatype is unrecognized.
    static std::unique_ptr<IIqSampleDecoder> create(const std::string& datatype);

    static bool is_supported(const std::string& datatype);
};

} // namespace module1
