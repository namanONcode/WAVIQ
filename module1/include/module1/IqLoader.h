#pragma once

#include "module1/ISignalLoader.h"
#include "module1/IqMetadata.h"

namespace module1 {

// Loads raw interleaved-IQ files (int8, int16, float32).
//
// Raw IQ has no header, so `load()` will:
//   1. look for a "<path>.sigmf-meta" sidecar next to the file, and use it
//      if present and valid;
//   2. otherwise throw MissingMetadataError -- it will NOT guess.
//
// When step 2 happens, the caller (GUI) collects the required fields from
// the user and calls load_with_metadata() directly; that path never
// touches the sidecar logic.
class IqLoader : public ISignalLoader {
public:
    ComplexSignal load(const std::string& path) const override;
    bool can_load(const std::string& path) const override;

    // Loads `path` using explicitly supplied metadata, bypassing sidecar
    // lookup entirely. Throws UnsupportedFormatError if `metadata` itself
    // is incomplete or invalid (use IqMetadataInput::missing_fields() to
    // validate on the GUI side before calling this).
    ComplexSignal load_with_metadata(const std::string& path, const IqMetadataInput& metadata) const;

private:
    ComplexSignal parse_raw(const std::string& path, const IqMetadataInput& metadata) const;
};

} // namespace module1
