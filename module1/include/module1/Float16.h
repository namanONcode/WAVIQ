#pragma once

#include <cstdint>
#include <cstring>

namespace module1 {

// Decodes a raw IEEE-754 binary16 ("half float") bit pattern into a float.
//
// This exists specifically to AVOID depending on HDF5's H5T_NATIVE_FLOAT16,
// which is only available in libhdf5 >= 1.14.4 and even then only when the
// library was built against a compiler with native _Float16 support (e.g.
// GCC 12+ on x86, not GCC 10). Reading the dataset's raw 16-bit words and
// decoding them ourselves works with any HDF5 version, including the
// 1.10.x / 1.12.x releases still shipped by current Ubuntu/Debian-family
// distributions (verified: this project's build environment has 1.10.10).
inline float half_to_float(uint16_t h) {
    const uint32_t sign = static_cast<uint32_t>(h & 0x8000u) << 16;
    uint32_t exponent = (h >> 10) & 0x1Fu;
    uint32_t mantissa = h & 0x3FFu;
    uint32_t bits;

    if (exponent == 0) {
        if (mantissa == 0) {
            bits = sign; // +/- zero
        } else {
            // Subnormal half -> normalize into a normal float.
            exponent = 127 - 15 + 1;
            while ((mantissa & 0x400u) == 0) {
                mantissa <<= 1;
                --exponent;
            }
            mantissa &= 0x3FFu;
            bits = sign | (exponent << 23) | (mantissa << 13);
        }
    } else if (exponent == 0x1Fu) {
        // Inf or NaN
        bits = sign | 0x7F800000u | (mantissa << 13);
    } else {
        // Normal half -> re-bias exponent from 15 to 127.
        bits = sign | ((exponent - 15 + 127) << 23) | (mantissa << 13);
    }

    float result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

} // namespace module1
