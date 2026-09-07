#pragma once

#include <memory>
#include <string>

#include "module1/ISignalLoader.h"

namespace module1 {

// Factory class responsible for routing input file paths to the appropriate
// ISignalLoader implementation based on file format / extension.
class LoaderFactory {
public:
    // Instantiates an ISignalLoader suitable for the given path.
    // Throws UnsupportedFormatError if the extension is not recognized (.wav, .iq).
    static std::unique_ptr<ISignalLoader> create_loader(const std::string& path);
};

// Backward-compatible convenience wrapper.
inline std::unique_ptr<ISignalLoader> make_loader_for(const std::string& path) {
    return LoaderFactory::create_loader(path);
}

} // namespace module1
