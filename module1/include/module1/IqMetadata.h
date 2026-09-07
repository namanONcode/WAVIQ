#pragma once

#include <optional>
#include <string>
#include <vector>

namespace module1 {

// The metadata required to interpret a raw IQ file, regardless of
// whether it came from a sidecar file or manual GUI entry -- both paths
// produce this same struct, so IqLoader doesn't care which one was used.
struct IqMetadataInput {
    double sample_rate_hz = 0.0;

    // One of: "int8", "int16", "float32". Empty means "not set".
    std::string sample_datatype;

    // Only "interleaved_iq" is supported today; kept as a string (not
    // wired to the enum) so unrecognized sidecar values can be reported
    // as a clear error instead of failing to parse.
    std::string iq_arrangement = "interleaved_iq";

    // "little" or "big". Irrelevant for int8 but still tracked so the
    // metadata's provenance is always fully recorded.
    std::string byte_order = "little";

    std::string source; // "sigmf_sidecar" or "manual_entry", set by caller

    bool is_valid() const { return missing_fields().empty(); }

    std::vector<std::string> missing_fields() const {
        std::vector<std::string> missing;
        if (sample_rate_hz <= 0.0) missing.push_back("sample_rate");
        if (sample_datatype.empty()) missing.push_back("sample_datatype");
        if (iq_arrangement != "interleaved_iq") missing.push_back("iq_arrangement (unsupported value)");
        if (byte_order != "little" && byte_order != "big") missing.push_back("byte_order");
        return missing;
    }
};

// Encapsulates discovery and parsing of SigMF metadata.
class SigmfMetadataParser {
public:
    // Looks for "<iq_path>.sigmf-meta" next to the given IQ file.
    // Returns std::nullopt if no sidecar file exists at that path.
    static std::optional<std::string> find_sidecar_path(const std::string& iq_path);

    // Parses a SigMF-style sidecar JSON file into an IqMetadataInput.
    // Throws FileIOError if the file can't be read.
    // Throws MalformedDataError if the JSON is malformed or missing required schema blocks.
    // Throws UnsupportedFormatError if core:datatype is unsupported.
    static IqMetadataInput parse_file(const std::string& sidecar_path);

    // Parses SigMF metadata directly from a JSON string.
    // Decouples schema parsing from filesystem I/O for unit testability.
    static IqMetadataInput parse_json(const std::string& json_content, const std::string& provenance = "sigmf_sidecar");
};

// Backward-compatible free functions:
inline std::optional<std::string> find_sidecar_path_for(const std::string& iq_path) {
    return SigmfMetadataParser::find_sidecar_path(iq_path);
}

inline IqMetadataInput parse_sigmf_sidecar(const std::string& sidecar_path) {
    return SigmfMetadataParser::parse_file(sidecar_path);
}

} // namespace module1
