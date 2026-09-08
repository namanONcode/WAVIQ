#include "TestFramework.hpp"
#include "RealDataLoader.hpp"
#include "preprocessing/Filter.hpp"
#include "core/SignalData.hpp"
#include <vector>
#include <iostream>

using namespace module2;

void run_filter_tests() {
    std::cout << "  [Filter] Running tests on real dataset files...\n";

    // Test 1: Moving Average Filter on Real WAV Data
    {
        core::SignalData data = test::RealDataLoader::loadWAV("real_subset.wav");
        size_t origSize = data.samples.size();
        test::check(origSize > 0, "Filter: Loaded real_subset.wav");

        std::vector<float> taps = {0.25f, 0.5f, 0.25f}; // 3-tap FIR filter
        preprocessing::Filter::applyFIR(data, taps);

        test::check(data.samples.size() == origSize, "Filter: Output size matches real WAV input size");
    }

    // Test 2: FIR Filter on Real IQ Data
    {
        core::SignalData data = test::RealDataLoader::loadIQFloat32("real_subset_float32.iq");
        size_t origSize = data.samples.size();
        test::check(origSize > 0, "Filter: Loaded real_subset_float32.iq");

        std::vector<float> taps = {0.5f, 0.5f}; // 2-tap moving average
        preprocessing::Filter::applyFIR(data, taps);

        test::check(data.samples.size() == origSize, "Filter: Output size matches real IQ input size");
    }
}
