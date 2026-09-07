#include "module1/IqLoader.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

#include "module1/LoaderExceptions.h"

namespace module1 {

namespace {

bool has_extension(const std::string& path, const std::string& ext) {
    if (path.size() < ext.size()) return false;
    std::string tail = path.substr(path.size() - ext.size());
    std::transform(tail.begin(), tail.end(), tail.begin(),
                    [](unsigned char c) { return std::tolower(c); });
    return tail == ext;
}

// Byte swaps. NOTE: these assume the host CPU is little-endian (true for
// the x86_64/ARM targets this project builds on) -- "need_swap" below is
// computed relative to that assumption, not detected at runtime.
int16_t swap_i16(int16_t v) {
    uint16_t u;
    std::memcpy(&u, &v, sizeof(u));
    u = static_cast<uint16_t>((u << 8) | (u >> 8));
    std::memcpy(&v, &u, sizeof(v));
    return v;
}

float swap_f32(float v) {
    uint32_t u;
    std::memcpy(&u, &v, sizeof(u));
    u = ((u & 0x000000FFu) << 24) | ((u & 0x0000FF00u) << 8) |
        ((u & 0x00FF0000u) >> 8) | ((u & 0xFF000000u) >> 24);
    std::memcpy(&v, &u, sizeof(v));
    return v;
}

} // namespace

bool IqLoader::can_load(const std::string& path) const {
    return has_extension(path, ".iq");
}

ComplexSignal IqLoader::load(const std::string& path) const {
    auto sidecar_path = find_sidecar_path_for(path);
    if (sidecar_path) {
        IqMetadataInput meta = parse_sigmf_sidecar(*sidecar_path);
        if (meta.is_valid()) {
            return load_with_metadata(path, meta);
        }
        // Sidecar exists but is incomplete -- still ask rather than
        // guessing at the missing pieces.
        throw MissingMetadataError(
            "Sidecar metadata at " + *sidecar_path + " is missing required fields.",
            meta.missing_fields());
    }

    throw MissingMetadataError(
        "No sidecar metadata found for " + path + " (expected " + path +
        ".sigmf-meta). Sample rate, sample datatype, IQ arrangement, and byte "
        "order must be provided manually.",
        {"sample_rate", "sample_datatype", "iq_arrangement", "byte_order"});
}

ComplexSignal IqLoader::load_with_metadata(const std::string& path, const IqMetadataInput& metadata) const {
    if (!metadata.is_valid()) {
        std::string joined;
        for (const auto& f : metadata.missing_fields()) {
            joined += (joined.empty() ? "" : ", ") + f;
        }
        throw UnsupportedFormatError("Supplied IQ metadata is incomplete, missing: " + joined);
    }
    return parse_raw(path, metadata);
}

ComplexSignal IqLoader::parse_raw(const std::string& path, const IqMetadataInput& metadata) const {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw FileIOError("Could not open raw IQ file: " + path);
    }

    in.seekg(0, std::ios::end);
    const std::streamsize file_size = in.tellg();
    in.seekg(0, std::ios::beg);

    const bool need_swap = (metadata.byte_order == "big");
    std::vector<std::complex<float>> samples;

    if (metadata.sample_datatype == "int8") {
        std::vector<int8_t> raw(static_cast<size_t>(file_size));
        in.read(reinterpret_cast<char*>(raw.data()), file_size);
        if (raw.size() % 2 != 0) {
            throw UnsupportedFormatError(
                "Raw IQ file has an odd byte count for int8 interleaved I/Q: " + path);
        }
        samples.reserve(raw.size() / 2);
        // Normalized to roughly [-1, 1], matching the float range libsndfile
        // already produces for WAV -- keeps both paths on comparable scales
        // for the visualizations downstream.
        for (size_t i = 0; i + 1 < raw.size(); i += 2) {
            samples.emplace_back(raw[i] / 127.0f, raw[i + 1] / 127.0f);
        }
    } else if (metadata.sample_datatype == "int16") {
        const size_t sample_bytes = static_cast<size_t>(file_size);
        if (sample_bytes % 4 != 0) { // 2 bytes I + 2 bytes Q per complex sample
            throw UnsupportedFormatError(
                "Raw IQ file size is not a multiple of 4 bytes, required for int16 "
                "interleaved I/Q: " + path);
        }
        std::vector<int16_t> raw(sample_bytes / 2);
        in.read(reinterpret_cast<char*>(raw.data()), file_size);
        samples.reserve(raw.size() / 2);
        for (size_t i = 0; i + 1 < raw.size(); i += 2) {
            int16_t i_raw = raw[i];
            int16_t q_raw = raw[i + 1];
            if (need_swap) { i_raw = swap_i16(i_raw); q_raw = swap_i16(q_raw); }
            samples.emplace_back(i_raw / 32768.0f, q_raw / 32768.0f);
        }
    } else if (metadata.sample_datatype == "float32") {
        const size_t sample_bytes = static_cast<size_t>(file_size);
        if (sample_bytes % 8 != 0) { // 4 bytes I + 4 bytes Q per complex sample
            throw UnsupportedFormatError(
                "Raw IQ file size is not a multiple of 8 bytes, required for float32 "
                "interleaved I/Q: " + path);
        }
        std::vector<float> raw(sample_bytes / 4);
        in.read(reinterpret_cast<char*>(raw.data()), file_size);
        samples.reserve(raw.size() / 2);
        for (size_t i = 0; i + 1 < raw.size(); i += 2) {
            float i_val = raw[i];
            float q_val = raw[i + 1];
            if (need_swap) { i_val = swap_f32(i_val); q_val = swap_f32(q_val); }
            samples.emplace_back(i_val, q_val);
        }
    } else {
        throw UnsupportedFormatError(
            "Unsupported IQ sample datatype '" + metadata.sample_datatype +
            "' (supported: int8, int16, float32): " + path);
    }

    SignalMetadata out_meta;
    out_meta.sample_rate_hz = metadata.sample_rate_hz;
    out_meta.source_format = "iq";
    out_meta.sample_datatype = metadata.sample_datatype;
    out_meta.channel_count = 2; // I and Q
    out_meta.is_complex = true;
    out_meta.iq_arrangement = IqArrangement::InterleavedIQ;
    out_meta.byte_order = (metadata.byte_order == "big") ? ByteOrder::Big : ByteOrder::Little;
    out_meta.source_path = path;
    out_meta.metadata_source = metadata.source.empty() ? "manual_entry" : metadata.source;

    return ComplexSignal(std::move(samples), std::move(out_meta));
}

} // namespace module1
