#include "TestFramework.hpp"
#include "RealDataLoader.hpp"
#include "preprocessing/SignalDetector.hpp"
#include "core/SignalData.hpp"
#include <vector>
#include <iostream>

using namespace module2;

void run_signaldetector_tests() {
    std::cout << "  [SignalDetector] Running tests on real dataset files...\n";

    // Test 1: Real WAV File Active Region Detection
    {
        core::SignalData data = test::RealDataLoader::loadWAV("real_subset.wav");
        test::check(!data.samples.empty(), "SignalDetector: Real WAV data loaded");

        bool found = preprocessing::SignalDetector::extractActiveRegion(data, 2.0f);
        test::check(found, "SignalDetector: Extracted active region from real_subset.wav");
        test::check(!data.samples.empty(), "SignalDetector: Extracted samples vector non-empty");
    }

    // Test 2: Real IQ File Active Region Detection
    {
        core::SignalData data = test::RealDataLoader::loadIQFloat32("real_subset_float32.iq");
        test::check(!data.samples.empty(), "SignalDetector: Real IQ data loaded");

        bool found = preprocessing::SignalDetector::extractActiveRegion(data, 2.0f);
        test::check(found, "SignalDetector: Extracted active region from real_subset_float32.iq");
        test::check(!data.samples.empty(), "SignalDetector: Extracted IQ samples vector non-empty");
    }
}
