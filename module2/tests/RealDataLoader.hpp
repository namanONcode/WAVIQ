#pragma once

#include "core/SignalData.hpp"
#include "core/Module2Exceptions.hpp"
#include <fstream>
#include <vector>
#include <complex>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sndfile.h>

namespace module2 {
namespace test {

class RealDataLoader {
public:
    static std::string getTestDataDir() {
        return "/home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/";
    }

    static core::SignalData loadIQFloat32(const std::string& filename, double sampleRate = 1000000.0) {
        std::string fullPath = getTestDataDir() + filename;
        std::ifstream file(fullPath, std::ios::binary);
        if (!file.is_open()) {
            throw DataLoaderError("Cannot open IQ file at " + fullPath);
        }

        file.seekg(0, std::ios::end);
        std::streamsize fileSize = file.tellg();
        if (fileSize == -1) {
            throw DataLoaderError("Cannot determine file size for " + fullPath);
        }
        
        if (fileSize % (sizeof(float) * 2) != 0) {
            throw DataLoaderError("IQ file size is not a multiple of complex float size in " + fullPath);
        }

        file.seekg(0, std::ios::beg);

        size_t numSamples = fileSize / (sizeof(float) * 2);
        std::vector<float> raw(numSamples * 2);
        
        file.read(reinterpret_cast<char*>(raw.data()), fileSize);
        
        if (file.gcount() != fileSize) {
            throw DataLoaderError("Premature EOF or read error in " + fullPath);
        }

        core::SignalData data;
        data.sampleRate = sampleRate;
        data.centerFrequency = 0.0;
        data.samples.resize(numSamples);
        
        for (size_t i = 0; i < numSamples; ++i) {
            data.samples[i] = std::complex<float>(raw[2 * i], raw[2 * i + 1]);
        }

        return data;
    }

    /**
     * Loads WAV file (e.g. real_subset.wav, real_subset_mono.wav) using libsndfile.
     */
    static core::SignalData loadWAV(const std::string& filename) {
        std::string fullPath = getTestDataDir() + filename;
        
        SF_INFO sfInfo = {};
        SNDFILE* file = sf_open(fullPath.c_str(), SFM_READ, &sfInfo);
        
        if (!file) {
            throw DataLoaderError("Cannot open WAV file at " + fullPath + " (libsndfile error)");
        }
        
        core::SignalData data;
        data.sampleRate = static_cast<double>(sfInfo.samplerate);
        data.centerFrequency = 0.0;
        
        std::vector<float> buffer(sfInfo.frames * sfInfo.channels);
        sf_count_t readFrames = sf_readf_float(file, buffer.data(), sfInfo.frames);
        
        sf_close(file);
        
        if (readFrames != sfInfo.frames) {
            throw DataLoaderError("Premature EOF reading WAV file via libsndfile");
        }
        
        if (sfInfo.channels == 1) {
            data.samples.resize(readFrames);
            for (sf_count_t i = 0; i < readFrames; ++i) {
                data.samples[i] = std::complex<float>(buffer[i], 0.0f);
            }
        } else if (sfInfo.channels == 2) {
            data.samples.resize(readFrames);
            for (sf_count_t i = 0; i < readFrames; ++i) {
                data.samples[i] = std::complex<float>(buffer[2 * i], buffer[2 * i + 1]);
            }
        } else {
            throw DataLoaderError("Unsupported number of channels (" + std::to_string(sfInfo.channels) + ")");
        }
        
        return data;
    }
};

} // namespace test
} // namespace module2
