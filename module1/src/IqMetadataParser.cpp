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
    if (sigmf_type == "ci8") { out_datatype = "int8"; out_byte_order = "little"; return true; }
    return false;
}

} // namespace

std::optional<std::string> find_sidecar_path_for(const std::string& iq_path) {
    const std::string candidate = iq_path + ".sigmf-meta";
    if (file_exists(candidate)) {
        return candidate;
    }
    return std::nullopt;
}

IqMetadataInput parse_sigmf_sidecar(const std::string& sidecar_path) {
    std::ifstream in(sidecar_path);
    if (!in) {
        throw FileIOError("Could not open sidecar metadata file: " + sidecar_path);
    }

    nlohmann::json j;
    try {
        in >> j;
    } catch (const nlohmann::json::parse_error& e) {
        throw UnsupportedFormatError(
            "Sidecar metadata file is not valid JSON: " + sidecar_path + " (" + e.what() + ")");
    }

    if (!j.contains("global") || !j["global"].is_object()) {
        throw UnsupportedFormatError(
            "Sidecar metadata file is missing the SigMF 'global' object: " + sidecar_path);
    }
    const auto& global = j["global"];

    IqMetadataInput meta;
    meta.source = "sigmf_sidecar";

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
                "cf32_le, cf32_be, ci16_le, ci16_be, ci8): " + sidecar_path);
        }
    }

    // iq_arrangement is left at its default ("interleaved_iq") since SigMF's
    // "c"-prefixed datatypes are interleaved IQ by definition -- there is
    // nothing else to read from the sidecar for this field.

    return meta;
}

} // namespace module1
