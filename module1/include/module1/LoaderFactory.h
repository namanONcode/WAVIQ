#pragma once

#include <memory>
#include <string>

#include "module1/ISignalLoader.h"

namespace module1 {

// Picks the right loader based on file extension.
// Throws UnsupportedFormatError if the extension isn't .wav or .iq.
std::unique_ptr<ISignalLoader> make_loader_for(const std::string& path);

} // namespace module1
