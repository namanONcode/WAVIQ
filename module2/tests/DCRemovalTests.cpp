#include "TestFramework.hpp"
#include "preprocessing/DCRemoval.hpp"
#include "core/SignalData.hpp"

using namespace module2;

void run_dcremoval_tests() {
    // Test 1: RemovesPositiveDCBias
    {
        core::SignalData data;
        data.sampleRate = 1000.0;
        data.centerFrequency = 0.0;
        
        for (int i = 0; i < 100; ++i) {
            data.samples.push_back({5.0f, 3.0f});
        }
        
        preprocessing::DCRemoval::process(data);
        
        for (const auto& sample : data.samples) {
            test::approx_check(sample.real(), 0.0f, "DCRemoval: Real part should be 0", 1e-5f);
            test::approx_check(sample.imag(), 0.0f, "DCRemoval: Imag part should be 0", 1e-5f);
        }
    }
    
    // Test 2: HandlesZeroMeanSignal
    {
        core::SignalData data;
        data.sampleRate = 1000.0;
        data.centerFrequency = 0.0;
        
        for (int i = 0; i < 100; ++i) {
            float val = (i % 2 == 0) ? 1.0f : -1.0f;
            data.samples.push_back({val, val});
        }
        
        preprocessing::DCRemoval::process(data);
        
        for (int i = 0; i < 100; ++i) {
            float expected = (i % 2 == 0) ? 1.0f : -1.0f;
            test::approx_check(data.samples[i].real(), expected, "DCRemoval: Zero mean signal unaltered real", 1e-5f);
            test::approx_check(data.samples[i].imag(), expected, "DCRemoval: Zero mean signal unaltered imag", 1e-5f);
        }
    }
    
    // Test 3: EmptySignal
    {
        core::SignalData data;
        data.sampleRate = 1000.0;
        data.centerFrequency = 0.0;
        
        // Should not crash
        preprocessing::DCRemoval::process(data);
        test::check(data.samples.empty(), "DCRemoval: Empty signal remains empty");
    }
}
