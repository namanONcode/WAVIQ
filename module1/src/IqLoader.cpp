#include "module1/IqLoader.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <vector>

#include "module1/IqSampleDecoder.h"
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

} // namespace

bool IqLoader::can_load(const std::string& path) const {
    return has_extension(path, ".iq");
}

ComplexSignal IqLoader::load(const std::string& path) const {
    auto sidecar_path = SigmfMetadataParser::find_sidecar_path(path);
    if (sidecar_path) {
        IqMetadataInput meta = SigmfMetadataParser::parse_file(*sidecar_path);
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

    std::unique_ptr<IIqSampleDecoder> decoder;
    try {
        decoder = IqSampleDecoderFactory::create(metadata.sample_datatype);
    } catch (const UnsupportedFormatError&) {
        throw UnsupportedFormatError(
            "Unsupported IQ sample datatype '" + metadata.sample_datatype +
            "' (supported: int8, int16, float32): " + path);
    }

    const size_t bytes_per_sample = decoder->bytes_per_sample();
    const size_t total_bytes = static_cast<size_t>(file_size);
    if (total_bytes % bytes_per_sample != 0) {
        if (metadata.sample_datatype == "int8") {
            throw UnsupportedFormatError(
                "Raw IQ file has an odd byte count for int8 interleaved I/Q: " + path);
        } else {
            throw UnsupportedFormatError(
                "Raw IQ file size is not a multiple of " + std::to_string(bytes_per_sample) +
                " bytes, required for " + metadata.sample_datatype +
                " interleaved I/Q: " + path);
        }
    }

    std::vector<uint8_t> raw(total_bytes);
    in.read(reinterpret_cast<char*>(raw.data()), file_size);

    const ByteOrder byte_order = (metadata.byte_order == "big") ? ByteOrder::Big : ByteOrder::Little;
    std::vector<std::complex<float>> samples = decoder->decode(raw.data(), raw.size(), byte_order);

    SignalMetadata out_meta;
    out_meta.sample_rate_hz = metadata.sample_rate_hz;
    out_meta.source_format = "iq";
    out_meta.sample_datatype = metadata.sample_datatype;
    out_meta.channel_count = 2; // I and Q
    out_meta.is_complex = true;
    out_meta.iq_arrangement = IqArrangement::InterleavedIQ;
    out_meta.byte_order = byte_order;
    out_meta.source_path = path;
    out_meta.metadata_source = metadata.source.empty() ? "manual_entry" : metadata.source;

    return ComplexSignal(std::move(samples), std::move(out_meta));
}

} // namespace module1
