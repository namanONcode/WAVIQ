#include "TestFramework.hpp"
#include "preprocessing/Filter.hpp"
#include "core/SignalData.hpp"
#include <vector>

using namespace module2;

void run_filter_tests() {
    // Test 1: AppliesMovingAverage
    {
        core::SignalData data;
        data.samples = {
            {1.0f, 0.0f}, {2.0f, 0.0f}, {3.0f, 0.0f}, {4.0f, 0.0f}
        };
        
        // 2-tap moving average filter
        std::vector<float> taps = {0.5f, 0.5f};
        
        preprocessing::Filter::applyFIR(data, taps);
        
        test::check(data.samples.size() == 4, "Filter: Moving average size");
        test::approx_check(data.samples[0].real(), 0.5f, "Filter: MA real 0", 1e-5f);
        test::approx_check(data.samples[1].real(), 1.5f, "Filter: MA real 1", 1e-5f);
        test::approx_check(data.samples[2].real(), 2.5f, "Filter: MA real 2", 1e-5f);
        test::approx_check(data.samples[3].real(), 3.5f, "Filter: MA real 3", 1e-5f);
    }
    
    // Test 2: AppliesImpulse
    {
        core::SignalData data;
        data.samples = {
            {1.0f, 2.0f}, {3.0f, 4.0f}, {5.0f, 6.0f}
        };
        
        // Identity filter
        std::vector<float> taps = {1.0f};
        
        preprocessing::Filter::applyFIR(data, taps);
        
        test::check(data.samples.size() == 3, "Filter: Impulse size");
        test::approx_check(data.samples[0].real(), 1.0f, "Filter: Impulse real 0", 1e-5f);
        test::approx_check(data.samples[0].imag(), 2.0f, "Filter: Impulse imag 0", 1e-5f);
        test::approx_check(data.samples[1].real(), 3.0f, "Filter: Impulse real 1", 1e-5f);
        test::approx_check(data.samples[1].imag(), 4.0f, "Filter: Impulse imag 1", 1e-5f);
        test::approx_check(data.samples[2].real(), 5.0f, "Filter: Impulse real 2", 1e-5f);
        test::approx_check(data.samples[2].imag(), 6.0f, "Filter: Impulse imag 2", 1e-5f);
    }
    
    // Test 3: EmptySignalOrTaps
    {
        core::SignalData data;
        std::vector<float> taps = {1.0f, 0.5f};
        
        preprocessing::Filter::applyFIR(data, taps);
        test::check(data.samples.empty(), "Filter: Empty signal remains empty");
        
        data.samples = {{1.0f, 0.0f}, {2.0f, 0.0f}};
        std::vector<float> empty_taps;
        
        preprocessing::Filter::applyFIR(data, empty_taps);
        // Assuming it doesn't crash, we're good.
    }
}
