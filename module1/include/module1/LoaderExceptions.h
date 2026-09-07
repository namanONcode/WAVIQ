#pragma once

#include <stdexcept>
#include <string>
#include <vector>

namespace module1 {

// Base class for every error a loader can raise. Callers (the GUI layer)
// can catch this alone if they don't need to distinguish cases.
class SignalLoaderError : public std::runtime_error {
public:
    explicit SignalLoaderError(const std::string& msg) : std::runtime_error(msg) {}
};

// The file extension/content isn't one of the formats this application
// supports (see the file-format contract). Not for "bytes we can't
// figure out how to interpret" -- that's MissingMetadataError.
class UnsupportedFormatError : public SignalLoaderError {
public:
    explicit UnsupportedFormatError(const std::string& msg) : SignalLoaderError(msg) {}
};

// The file couldn't be opened/read (missing, permissions, truncated).
class FileIOError : public SignalLoaderError {
public:
    explicit FileIOError(const std::string& msg) : SignalLoaderError(msg) {}
};

// Raised when a raw IQ file has no usable sidecar metadata and none was
// supplied manually. Per the contract, the application must not guess --
// it must ask. missing_fields() lets the caller (GUI) build a targeted
// form instead of a generic error dialog, and IqLoader::load_with_metadata()
// is the retry path once the user fills it in.
class MissingMetadataError : public SignalLoaderError {
public:
    MissingMetadataError(const std::string& msg, std::vector<std::string> missing_fields)
        : SignalLoaderError(msg), missing_fields_(std::move(missing_fields)) {}

    const std::vector<std::string>& missing_fields() const { return missing_fields_; }

private:
    std::vector<std::string> missing_fields_;
};

} // namespace module1
