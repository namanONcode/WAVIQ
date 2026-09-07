#pragma once

#include <complex>
#include <vector>

#include "module1/ISignalLoader.h"

namespace module1 {

// Loads mono (real-valued) and 2-channel (I/Q) WAV files. All format
// information (sample rate, bit depth, channel count) comes from the WAV
// header itself -- no sidecar or manual entry is ever needed for WAV.
class WavLoader : public ISignalLoader {
public:
    ComplexSignal load(const std::string& path) const override;
    bool can_load(const std::string& path) const override;

private:
    static std::vector<std::complex<float>> decode_mono_samples(
        const std::vector<float>& interleaved, size_t frame_count);

    static std::vector<std::complex<float>> decode_stereo_iq_samples(
        const std::vector<float>& interleaved, size_t frame_count);

    static SignalMetadata build_metadata(
        double sample_rate_hz, int channels, const std::string& datatype_label, const std::string& path);
};

} // namespace module1
