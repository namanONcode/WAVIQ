#include "TestFramework.hpp"
#include "preprocessing/Normalizer.hpp"
#include "core/SignalData.hpp"
#include <cmath>

using namespace module2;

void run_normalizer_tests() {
    // Test 1: ScalesUpSignal
    {
        core::SignalData data;
        data.samples.push_back({0.3f, 0.4f}); // Max absolute value will be 0.5
        data.samples.push_back({0.1f, 0.1f});
        
        preprocessing::Normalizer::process(data);
        
        test::approx_check(data.samples[0].real(), 0.6f, "Normalizer: scales up real 0", 1e-5f);
        test::approx_check(data.samples[0].imag(), 0.8f, "Normalizer: scales up imag 0", 1e-5f);
        test::approx_check(data.samples[1].real(), 0.2f, "Normalizer: scales up real 1", 1e-5f);
        test::approx_check(data.samples[1].imag(), 0.2f, "Normalizer: scales up imag 1", 1e-5f);
    }

    // Test 2: ScalesDownSignal
    {
        core::SignalData data;
        data.samples.push_back({3.0f, 4.0f}); // Max absolute value will be 5.0
        data.samples.push_back({1.5f, 2.0f});
        
        preprocessing::Normalizer::process(data);
        
        test::approx_check(data.samples[0].real(), 0.6f, "Normalizer: scales down real 0", 1e-5f);
        test::approx_check(data.samples[0].imag(), 0.8f, "Normalizer: scales down imag 0", 1e-5f);
        test::approx_check(data.samples[1].real(), 0.3f, "Normalizer: scales down real 1", 1e-5f);
        test::approx_check(data.samples[1].imag(), 0.4f, "Normalizer: scales down imag 1", 1e-5f);
    }
    
    // Test 3: HandlesZeroSignal
    {
        core::SignalData data;
        data.samples.push_back({0.0f, 0.0f});
        data.samples.push_back({0.0f, 0.0f});
        
        preprocessing::Normalizer::process(data);
        
        test::approx_check(data.samples[0].real(), 0.0f, "Normalizer: zero signal real 0", 1e-5f);
        test::approx_check(data.samples[0].imag(), 0.0f, "Normalizer: zero signal imag 0", 1e-5f);
    }
    
    // Test 4: EmptySignal
    {
        core::SignalData data;
        preprocessing::Normalizer::process(data);
        test::check(data.samples.empty(), "Normalizer: Empty signal remains empty");
    }
}
