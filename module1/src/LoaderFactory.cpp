#include "module1/LoaderFactory.h"

#include "module1/IqLoader.h"
#include "module1/LoaderExceptions.h"
#include "module1/WavLoader.h"

namespace module1 {

std::unique_ptr<ISignalLoader> LoaderFactory::create_loader(const std::string& path) {
    auto wav = std::make_unique<WavLoader>();
    if (wav->can_load(path)) {
        return wav;
    }

    auto iq = std::make_unique<IqLoader>();
    if (iq->can_load(path)) {
        return iq;
    }

    throw UnsupportedFormatError(
        "Unrecognized file extension for: " + path + " (supported: .wav, .iq)");
}

} // namespace module1
