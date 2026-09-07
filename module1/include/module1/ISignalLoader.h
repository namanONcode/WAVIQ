#pragma once

#include <string>

#include "module1/SignalTypes.h"

namespace module1 {

class ISignalLoader {
public:
    virtual ~ISignalLoader() = default;

    // Loads `path` into the common ComplexSignal representation.
    // Throws UnsupportedFormatError, FileIOError, or (for raw IQ inputs
    // with no usable metadata) MissingMetadataError.
    virtual ComplexSignal load(const std::string& path) const = 0;

    // Cheap check used by the loader factory to route by extension.
    // Does not open the file.
    virtual bool can_load(const std::string& path) const = 0;
};

} // namespace module1
