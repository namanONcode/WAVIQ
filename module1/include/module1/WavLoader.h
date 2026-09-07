#pragma once

#include "module1/ISignalLoader.h"

namespace module1 {

// Loads mono (real-valued) and 2-channel (I/Q) WAV files. All format
// information (sample rate, bit depth, channel count) comes from the WAV
// header itself -- no sidecar or manual entry is ever needed for WAV.
class WavLoader : public ISignalLoader {
public:
    ComplexSignal load(const std::string& path) const override;
    bool can_load(const std::string& path) const override;
};

} // namespace module1
