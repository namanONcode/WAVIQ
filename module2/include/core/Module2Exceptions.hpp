#pragma once
#include <stdexcept>
#include <string>

namespace module2 {

/**
 * @brief Base exception class for all Module 2 (Signal Processing) errors.
 */
class SignalAnalyzerError : public std::runtime_error {
public:
    explicit SignalAnalyzerError(const std::string& message) 
        : std::runtime_error(message) {}
};

/**
 * @brief Thrown when DataLoader encounters file errors.
 */
class DataLoaderError : public SignalAnalyzerError {
public:
    explicit DataLoaderError(const std::string& message) 
        : SignalAnalyzerError("Data Loader Error: " + message) {}
};

/**
 * @brief Thrown when DSP pipeline algorithms fail (e.g. unsupported params).
 */
class DspProcessingError : public SignalAnalyzerError {
public:
    explicit DspProcessingError(const std::string& message) 
        : SignalAnalyzerError("DSP Processing Error: " + message) {}
};

} // namespace module2
