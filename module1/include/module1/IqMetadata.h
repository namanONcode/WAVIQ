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

// Looks for "<iq_path>.sigmf-meta" next to the given IQ file.
// Returns std::nullopt if no sidecar file exists at that path (this is
// not an error -- absence just means "fall back to manual entry").
std::optional<std::string> find_sidecar_path_for(const std::string& iq_path);

// Parses a SigMF-style sidecar JSON file (a "global" object containing
// "core:sample_rate" and "core:datatype") into an IqMetadataInput.
//
// Supported core:datatype values: cf32_le, cf32_be, ci16_le, ci16_be, ci8.
// (cu8 -- unsigned 8-bit -- is common on RTL-SDR captures but is not yet
// in the supported-formats list; see IqMetadataParser.cpp for the mapping
// table if that needs to be added later.)
//
// Throws FileIOError if the file can't be read, UnsupportedFormatError if
// the JSON is malformed or core:datatype isn't one of the values above.
IqMetadataInput parse_sigmf_sidecar(const std::string& sidecar_path);

} // namespace module1
