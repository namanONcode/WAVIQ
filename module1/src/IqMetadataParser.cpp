#include "module1/IqMetadata.h"

#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include <nlohmann/json.hpp>

#include "module1/LoaderExceptions.h"

namespace module1 {

namespace {

bool file_exists(const std::string& path) {
    struct stat buffer{};
    return stat(path.c_str(), &buffer) == 0;
}

// Maps a SigMF core:datatype string to (sample_datatype, byte_order).
// Returns false if the datatype string isn't one this application supports.
bool map_sigmf_datatype(const std::string& sigmf_type, std::string& out_datatype, std::string& out_byte_order) {
    if (sigmf_type == "cf32_le") { out_datatype = "float32"; out_byte_order = "little"; return true; }
    if (sigmf_type == "cf32_be") { out_datatype = "float32"; out_byte_order = "big"; return true; }
    if (sigmf_type == "ci16_le") { out_datatype = "int16"; out_byte_order = "little"; return true; }
    if (sigmf_type == "ci16_be") { out_datatype = "int16"; out_byte_order = "big"; return true; }
    if (sigmf_type == "ci8")     { out_datatype = "int8";    out_byte_order = "little"; return true; }
    return false;
}

} // namespace

std::optional<std::string> SigmfMetadataParser::find_sidecar_path(const std::string& iq_path) {
    const std::string candidate = iq_path + ".sigmf-meta";
    if (file_exists(candidate)) {
        return candidate;
    }
    return std::nullopt;
}

IqMetadataInput SigmfMetadataParser::parse_file(const std::string& sidecar_path) {
    std::ifstream in(sidecar_path);
    if (!in) {
        throw FileIOError("Could not open sidecar metadata file: " + sidecar_path);
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    try {
        return parse_json(buffer.str(), "sigmf_sidecar");
    } catch (const MalformedDataError& e) {
        throw MalformedDataError(std::string(e.what()) + ": " + sidecar_path);
    } catch (const UnsupportedFormatError& e) {
        // Re-throw with filename context for clarity
        throw UnsupportedFormatError(std::string(e.what()) + ": " + sidecar_path);
    }
}

IqMetadataInput SigmfMetadataParser::parse_json(const std::string& json_content, const std::string& provenance) {
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(json_content);
    } catch (const nlohmann::json::parse_error& e) {
        throw MalformedDataError("Sidecar metadata is not valid JSON (" + std::string(e.what()) + ")");
    }

    if (!j.contains("global") || !j["global"].is_object()) {
        throw MalformedDataError("Sidecar metadata is missing the SigMF 'global' object");
    }
    const auto& global = j["global"];

    IqMetadataInput meta;
    meta.source = provenance;

    if (global.contains("core:sample_rate") && global["core:sample_rate"].is_number()) {
        meta.sample_rate_hz = global["core:sample_rate"].get<double>();
    }

    if (global.contains("core:datatype") && global["core:datatype"].is_string()) {
        const std::string sigmf_type = global["core:datatype"].get<std::string>();
        std::string datatype, byte_order;
        if (map_sigmf_datatype(sigmf_type, datatype, byte_order)) {
            meta.sample_datatype = datatype;
            meta.byte_order = byte_order;
        } else {
            throw UnsupportedFormatError(
                "Sidecar declares core:datatype '" + sigmf_type +
                "', which this application does not support (supported: "
                "cf32_le, cf32_be, ci16_le, ci16_be, ci8)");
        }
    }

    // iq_arrangement is left at its default ("interleaved_iq") since SigMF's
    // "c"-prefixed datatypes are interleaved IQ by definition.

    return meta;
}

} // namespace module1
