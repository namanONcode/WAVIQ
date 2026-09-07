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

// Raised when a file claims or appears to be a supported format, but its
// internal structure, schema, header, or attributes are corrupt or malformed.
// Distinct from UnsupportedFormatError (which means the format itself is not supported).
class MalformedDataError : public SignalLoaderError {
public:
    explicit MalformedDataError(const std::string& msg) : SignalLoaderError(msg) {}
};

// Raised when an HDF5 dataset file fails schema validation (e.g. missing required
// datasets or attributes, attribute/dimension mismatches, unparseable mod2id JSON).
class Hdf5MalformedDatasetError : public MalformedDataError {
public:
    explicit Hdf5MalformedDatasetError(const std::string& msg) : MalformedDataError(msg) {}
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

// Raised when attempting to access a dataset frame index outside the valid range [0, frame_count - 1].
// Always carries the requested_index and frame_count as structured fields.
class FrameIndexOutOfRangeError : public SignalLoaderError {
public:
    FrameIndexOutOfRangeError(size_t requested_index, size_t frame_count)
        : SignalLoaderError("frame_index " + std::to_string(requested_index) +
                            " out of range (frame_count = " + std::to_string(frame_count) + ")"),
          requested_index_(requested_index),
          frame_count_(frame_count) {}

    size_t requested_index() const { return requested_index_; }
    size_t frame_count() const { return frame_count_; }

private:
    size_t requested_index_ = 0;
    size_t frame_count_ = 0;
};

} // namespace module1
