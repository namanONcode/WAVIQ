#include "TestFramework.hpp"
#include "RealDataLoader.hpp"
#include "preprocessing/DCRemoval.hpp"
#include "core/SignalData.hpp"
#include <numeric>
#include <iostream>

using namespace module2;

void run_dcremoval_tests() {
    std::cout << "  [DCRemoval] Running tests on real dataset files...\n";

    // Test 1: Real WAV File (real_subset.wav)
    {
        core::SignalData data = test::RealDataLoader::loadWAV("real_subset.wav");
        test::check(!data.samples.empty(), "DCRemoval: Real WAV data loaded successfully");

        preprocessing::DCRemoval::process(data);

        std::complex<float> sum(0.0f, 0.0f);
        for (const auto& sample : data.samples) {
            sum += sample;
        }
        std::complex<float> mean = sum / static_cast<float>(data.samples.size());

        test::approx_check(mean.real(), 0.0f, "DCRemoval: Real WAV mean real part should be ~0", 1e-4f);
        test::approx_check(mean.imag(), 0.0f, "DCRemoval: Real WAV mean imag part should be ~0", 1e-4f);
    }

    // Test 2: Real Mono WAV File (real_subset_mono.wav)
    {
        core::SignalData data = test::RealDataLoader::loadWAV("real_subset_mono.wav");
        test::check(!data.samples.empty(), "DCRemoval: Real Mono WAV data loaded successfully");

        preprocessing::DCRemoval::process(data);

        std::complex<float> sum(0.0f, 0.0f);
        for (const auto& sample : data.samples) {
            sum += sample;
        }
        std::complex<float> mean = sum / static_cast<float>(data.samples.size());

        test::approx_check(mean.real(), 0.0f, "DCRemoval: Mono WAV mean real part should be ~0", 1e-4f);
    }

    // Test 3: Real IQ File (real_subset_float32.iq)
    {
        core::SignalData data = test::RealDataLoader::loadIQFloat32("real_subset_float32.iq");
        test::check(!data.samples.empty(), "DCRemoval: Real IQ Float32 data loaded successfully");

        preprocessing::DCRemoval::process(data);

        std::complex<float> sum(0.0f, 0.0f);
        for (const auto& sample : data.samples) {
            sum += sample;
        }
        std::complex<float> mean = sum / static_cast<float>(data.samples.size());

        test::approx_check(mean.real(), 0.0f, "DCRemoval: Real IQ mean real part should be ~0", 1e-4f);
        test::approx_check(mean.imag(), 0.0f, "DCRemoval: Real IQ mean imag part should be ~0", 1e-4f);
    }
}
