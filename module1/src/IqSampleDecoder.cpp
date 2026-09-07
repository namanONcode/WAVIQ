#include "module1/IqSampleDecoder.h"

#include <cstring>

#include "module1/LoaderExceptions.h"

namespace module1 {

namespace {

uint16_t swap_u16(uint16_t u) {
    return static_cast<uint16_t>((u << 8) | (u >> 8));
}

uint32_t swap_u32(uint32_t u) {
    return ((u & 0x000000FFu) << 24) | ((u & 0x0000FF00u) << 8) |
           ((u & 0x00FF0000u) >> 8)  | ((u & 0xFF000000u) >> 24);
}

const std::string kInt8Name = "int8";
const std::string kInt16Name = "int16";
const std::string kFloat32Name = "float32";

} // namespace

const std::string& Int8IqDecoder::datatype_name() const {
    return kInt8Name;
}

std::vector<std::complex<float>> Int8IqDecoder::decode(
    const uint8_t* data,
    size_t byte_count,
    ByteOrder /*byte_order*/) const {
    if (byte_count % 2 != 0) {
        throw UnsupportedFormatError("Byte count is not an even number, required for int8 interleaved I/Q.");
    }
    std::vector<std::complex<float>> samples;
    samples.reserve(byte_count / 2);

    for (size_t i = 0; i + 1 < byte_count; i += 2) {
        const auto i_raw = static_cast<int8_t>(data[i]);
        const auto q_raw = static_cast<int8_t>(data[i + 1]);
        samples.emplace_back(i_raw / 128.0f, q_raw / 128.0f);
    }
    return samples;
}

const std::string& Int16IqDecoder::datatype_name() const {
    return kInt16Name;
}

std::vector<std::complex<float>> Int16IqDecoder::decode(
    const uint8_t* data,
    size_t byte_count,
    ByteOrder byte_order) const {
    if (byte_count % 4 != 0) {
        throw UnsupportedFormatError("Byte count is not a multiple of 4 bytes, required for int16 interleaved I/Q.");
    }
    const bool need_swap = (byte_order == ByteOrder::Big);
    std::vector<std::complex<float>> samples;
    samples.reserve(byte_count / 4);

    for (size_t i = 0; i + 3 < byte_count; i += 4) {
        uint16_t i_u = 0, q_u = 0;
        std::memcpy(&i_u, data + i, sizeof(uint16_t));
        std::memcpy(&q_u, data + i + 2, sizeof(uint16_t));
        if (need_swap) {
            i_u = swap_u16(i_u);
            q_u = swap_u16(q_u);
        }
        int16_t i_raw = 0, q_raw = 0;
        std::memcpy(&i_raw, &i_u, sizeof(int16_t));
        std::memcpy(&q_raw, &q_u, sizeof(int16_t));
        samples.emplace_back(i_raw / 32768.0f, q_raw / 32768.0f);
    }
    return samples;
}

const std::string& Float32IqDecoder::datatype_name() const {
    return kFloat32Name;
}

std::vector<std::complex<float>> Float32IqDecoder::decode(
    const uint8_t* data,
    size_t byte_count,
    ByteOrder byte_order) const {
    if (byte_count % 8 != 0) {
        throw UnsupportedFormatError("Byte count is not a multiple of 8 bytes, required for float32 interleaved I/Q.");
    }
    const bool need_swap = (byte_order == ByteOrder::Big);
    std::vector<std::complex<float>> samples;
    samples.reserve(byte_count / 8);

    for (size_t i = 0; i + 7 < byte_count; i += 8) {
        uint32_t i_u = 0, q_u = 0;
        std::memcpy(&i_u, data + i, sizeof(uint32_t));
        std::memcpy(&q_u, data + i + 4, sizeof(uint32_t));
        if (need_swap) {
            i_u = swap_u32(i_u);
            q_u = swap_u32(q_u);
        }
        float i_val = 0.0f, q_val = 0.0f;
        std::memcpy(&i_val, &i_u, sizeof(float));
        std::memcpy(&q_val, &q_u, sizeof(float));
        samples.emplace_back(i_val, q_val);
    }
    return samples;
}

std::unique_ptr<IIqSampleDecoder> IqSampleDecoderFactory::create(const std::string& datatype) {
    if (datatype == "int8") {
        return std::make_unique<Int8IqDecoder>();
    }
    if (datatype == "int16") {
        return std::make_unique<Int16IqDecoder>();
    }
    if (datatype == "float32") {
        return std::make_unique<Float32IqDecoder>();
    }
    throw UnsupportedFormatError("Unsupported IQ sample datatype '" + datatype +
                                 "' (supported: int8, int16, float32)");
}

bool IqSampleDecoderFactory::is_supported(const std::string& datatype) {
    return datatype == "int8" || datatype == "int16" || datatype == "float32";
}

} // namespace module1
