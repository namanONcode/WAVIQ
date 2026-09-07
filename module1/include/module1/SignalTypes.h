#pragma once

#include <complex>
#include <string>
#include <vector>

namespace module1 {

// How raw IQ bytes are laid out. Only interleaved is in-scope per the
// current contract, but this is kept as an enum (not a bool) so adding
// planar (I-buffer, Q-buffer) support later doesn't change call sites.
enum class IqArrangement {
    InterleavedIQ, // I, Q, I, Q, ...
};

enum class ByteOrder {
    Little,
    Big,
};

// Describes the effective interpretation applied to a loaded file,
// regardless of whether that interpretation came from a WAV header,
// a SigMF-style sidecar, or manual GUI entry.
struct SignalMetadata {
    double sample_rate_hz = 0.0;

    // "wav" or "iq"
    std::string source_format;

    // Human-readable sample datatype label, e.g. "pcm16", "pcm24",
    // "float32", "int8", "int16".
    std::string sample_datatype;

    int channel_count = 0;

    // false for mono real-valued WAV; true for 2-channel WAV (I/Q) and
    // all raw IQ input.
    bool is_complex = false;

    IqArrangement iq_arrangement = IqArrangement::InterleavedIQ;
    ByteOrder byte_order = ByteOrder::Little;

    std::string source_path;

    // Where this metadata came from: "wav_header", "sigmf_sidecar",
    // or "manual_entry". Useful for the GUI to show provenance and for
    // debugging misinterpreted captures.
    std::string metadata_source;
};

// The common internal representation that every loader converts into.
// Everything downstream (Module 2, and Module 1's own visualizations)
// operates only on this type -- it never needs to know whether the
// samples originally came from a WAV file or a raw IQ file.
class ComplexSignal {
public:
    ComplexSignal() = default;
    ComplexSignal(std::vector<std::complex<float>> samples, SignalMetadata metadata)
        : samples_(std::move(samples)), metadata_(std::move(metadata)) {}

    const std::vector<std::complex<float>>& samples() const { return samples_; }
    std::vector<std::complex<float>>& samples() { return samples_; }

    size_t sample_count() const { return samples_.size(); }
    double sample_rate() const { return metadata_.sample_rate_hz; }
    const SignalMetadata& metadata() const { return metadata_; }

private:
    std::vector<std::complex<float>> samples_;
    SignalMetadata metadata_;
};

} // namespace module1
