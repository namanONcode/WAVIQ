#include "TestFramework.hpp"
#include "RealDataLoader.hpp"
#include "preprocessing/Normalizer.hpp"
#include "core/SignalData.hpp"
#include <cmath>
#include <iostream>
#include <algorithm>

using namespace module2;

void run_normalizer_tests() {
    std::cout << "  [Normalizer] Running tests on real dataset files...\n";

    // Test 1: Real WAV File (real_subset.wav)
    {
        core::SignalData data = test::RealDataLoader::loadWAV("real_subset.wav");
        test::check(!data.samples.empty(), "Normalizer: Real WAV data loaded");

        preprocessing::Normalizer::process(data);

        float maxMag = 0.0f;
        for (const auto& s : data.samples) {
            float mag = std::abs(s);
            if (mag > maxMag) maxMag = mag;
        }

        test::approx_check(maxMag, 1.0f, "Normalizer: Real WAV max magnitude scaled to 1.0", 1e-4f);
    }

    // Test 2: Real IQ File (real_subset_float32.iq)
    {
        core::SignalData data = test::RealDataLoader::loadIQFloat32("real_subset_float32.iq");
        test::check(!data.samples.empty(), "Normalizer: Real IQ data loaded");

        preprocessing::Normalizer::process(data);

        float maxMag = 0.0f;
        for (const auto& s : data.samples) {
            float mag = std::abs(s);
            if (mag > maxMag) maxMag = mag;
        }

        test::approx_check(maxMag, 1.0f, "Normalizer: Real IQ max magnitude scaled to 1.0", 1e-4f);
    }
}
