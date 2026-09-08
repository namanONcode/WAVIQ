#include "TestFramework.hpp"
#include "RealDataLoader.hpp"
#include "analysis/SymbolRateEstimator.hpp"
#include "core/SignalData.hpp"
#include <iostream>

using namespace module2::analysis;
using namespace module2::core;
using namespace module2::test;

void run_symbolrateestimator_tests() {
    std::cout << "  [SymbolRateEstimator] Running tests on real dataset files...\n";

    // Test 1: Real WAV File Symbol Rate Estimation
    {
        SignalData data = RealDataLoader::loadWAV("real_subset.wav");
        check(!data.samples.empty(), "SymbolRateEstimator: Loaded real_subset.wav");

        auto result = SymbolRateEstimator::estimate(data);
        check(result.estimatedRate > 0.0f, "SymbolRateEstimator: Estimated positive rate for real_subset.wav");
        check(!result.candidateRates.empty(), "SymbolRateEstimator: Extracted candidates for real_subset.wav");
    }

    // Test 2: Real Mono WAV File Symbol Rate Estimation
    {
        SignalData data = RealDataLoader::loadWAV("real_subset_mono.wav");
        check(!data.samples.empty(), "SymbolRateEstimator: Loaded real_subset_mono.wav");

        auto result = SymbolRateEstimator::estimateDelayMultiply(data);
        check(result.estimatedRate >= 0.0f, "SymbolRateEstimator: Real mono WAV estimation complete");
    }

    // Test 3: Real IQ Float32 File Symbol Rate Estimation
    {
        SignalData data = RealDataLoader::loadIQFloat32("real_subset_float32.iq");
        check(!data.samples.empty(), "SymbolRateEstimator: Loaded real_subset_float32.iq");

        auto result = SymbolRateEstimator::estimate(data);
        check(result.estimatedRate > 0.0f, "SymbolRateEstimator: Estimated positive rate for real_subset_float32.iq");
        check(!result.candidateRates.empty(), "SymbolRateEstimator: Extracted candidates for real_subset_float32.iq");
    }
}
