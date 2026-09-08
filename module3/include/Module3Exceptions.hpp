#pragma once
#include <stdexcept>
#include <string>

namespace module3 {

/**
 * @brief Base exception class for all Module 3 (Bitstream Processing) errors.
 */
class BitstreamProcessorError : public std::runtime_error {
public:
    explicit BitstreamProcessorError(const std::string& message) 
        : std::runtime_error(message) {}
};

/**
 * @brief Thrown when de-interleaving fails (e.g., mismatched dimensions).
 */
class DeinterleaverError : public BitstreamProcessorError {
public:
    explicit DeinterleaverError(const std::string& message) 
        : BitstreamProcessorError("Deinterleaver Error: " + message) {}
};

/**
 * @brief Thrown when Forward Error Correction decoding fails.
 */
class FecDecoderError : public BitstreamProcessorError {
public:
    explicit FecDecoderError(const std::string& message) 
        : BitstreamProcessorError("FEC Decoder Error: " + message) {}
};

/**
 * @brief Thrown when bitstream correlation (sync word matching) fails.
 */
class CorrelatorError : public BitstreamProcessorError {
public:
    explicit CorrelatorError(const std::string& message) 
        : BitstreamProcessorError("Correlator Error: " + message) {}
};

} // namespace module3
