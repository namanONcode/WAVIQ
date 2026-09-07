#include "module1/WavLoader.h"

#include <algorithm>
#include <cctype>
#include <vector>

#include <sndfile.h>

#include "module1/LoaderExceptions.h"

namespace module1 {

namespace {

bool has_extension(const std::string& path, const std::string& ext) {
    if (path.size() < ext.size()) return false;
    std::string tail = path.substr(path.size() - ext.size());
    std::transform(tail.begin(), tail.end(), tail.begin(),
                    [](unsigned char c) { return std::tolower(c); });
    return tail == ext;
}

// Maps libsndfile's subtype bitmask to the datatype labels this
// application supports. Returns false for anything else (e.g. PCM_S8,
// ULAW, ADPCM), which the loader rejects per the file-format contract.
bool map_wav_subtype(int format, std::string& out_label) {
    const int subtype = format & SF_FORMAT_SUBMASK;
    switch (subtype) {
        case SF_FORMAT_PCM_16: out_label = "pcm16"; return true;
        case SF_FORMAT_PCM_24: out_label = "pcm24"; return true;
        case SF_FORMAT_FLOAT:  out_label = "float32"; return true;
        default: return false;
    }
}

} // namespace

bool WavLoader::can_load(const std::string& path) const {
    return has_extension(path, ".wav");
}

ComplexSignal WavLoader::load(const std::string& path) const {
    SF_INFO sfinfo{};
    SNDFILE* file = sf_open(path.c_str(), SFM_READ, &sfinfo);
    if (!file) {
        throw FileIOError("Could not open WAV file: " + path + " (" + sf_strerror(nullptr) + ")");
    }

    std::string datatype_label;
    if (!map_wav_subtype(sfinfo.format, datatype_label)) {
        sf_close(file);
        throw UnsupportedFormatError(
            "WAV file uses an unsupported sample format (supported: PCM16, PCM24, "
            "Float32): " + path);
    }

    if (sfinfo.channels != 1 && sfinfo.channels != 2) {
        sf_close(file);
        throw UnsupportedFormatError(
            "WAV file has " + std::to_string(sfinfo.channels) +
            " channels; only mono (real-valued) and 2-channel (I/Q) are supported: " + path);
    }

    const sf_count_t frame_count = sfinfo.frames;
    std::vector<float> interleaved(static_cast<size_t>(frame_count) * sfinfo.channels);

    const sf_count_t frames_read = sf_readf_float(file, interleaved.data(), frame_count);
    sf_close(file);

    if (frames_read != frame_count) {
        throw FileIOError(
            "WAV file appears truncated: expected " + std::to_string(frame_count) +
            " frames, read " + std::to_string(frames_read) + ": " + path);
    }

    std::vector<std::complex<float>> samples;
    samples.reserve(static_cast<size_t>(frame_count));

    if (sfinfo.channels == 1) {
        for (sf_count_t i = 0; i < frame_count; ++i) {
            samples.emplace_back(interleaved[static_cast<size_t>(i)], 0.0f);
        }
    } else { // channels == 2: channel 0 = I, channel 1 = Q
        for (sf_count_t i = 0; i < frame_count; ++i) {
            const float i_val = interleaved[static_cast<size_t>(i) * 2 + 0];
            const float q_val = interleaved[static_cast<size_t>(i) * 2 + 1];
            samples.emplace_back(i_val, q_val);
        }
    }

    SignalMetadata metadata;
    metadata.sample_rate_hz = static_cast<double>(sfinfo.samplerate);
    metadata.source_format = "wav";
    metadata.sample_datatype = datatype_label;
    metadata.channel_count = sfinfo.channels;
    metadata.is_complex = (sfinfo.channels == 2);
    metadata.iq_arrangement = IqArrangement::InterleavedIQ;
    metadata.byte_order = ByteOrder::Little;
    metadata.source_path = path;
    metadata.metadata_source = "wav_header";

    return ComplexSignal(std::move(samples), std::move(metadata));
}

} // namespace module1
