#include "TestFramework.hpp"
#include "preprocessing/SignalDetector.hpp"
#include "core/SignalData.hpp"
#include <vector>

using namespace module2;

void run_signaldetector_tests() {
    // Test 1: ExtractsActiveRegion
    {
        core::SignalData data;
        
        // Add low energy noise (amplitude ~ 0.1)
        for (int i = 0; i < 50; ++i) {
            data.samples.push_back({0.1f, 0.0f});
        }
        
        // Add high energy pulse (amplitude ~ 10.0)
        for (int i = 0; i < 20; ++i) {
            data.samples.push_back({10.0f, 0.0f});
        }
        
        // Add low energy noise again
        for (int i = 0; i < 30; ++i) {
            data.samples.push_back({0.1f, 0.0f});
        }
        
        bool found = preprocessing::SignalDetector::extractActiveRegion(data, 3.0f);
        
        test::check(found, "SignalDetector: Extracts active region found");
        test::check(data.samples.size() >= 15 && data.samples.size() <= 25, "SignalDetector: Extracted size near 20");
    }
    
    // Test 2: PureNoise
    {
        core::SignalData data;
        
        // Add low energy noise only
        for (int i = 0; i < 100; ++i) {
            data.samples.push_back({0.1f, 0.1f});
        }
        
        bool found = preprocessing::SignalDetector::extractActiveRegion(data, 5.0f);
        
        test::check(!found, "SignalDetector: Pure noise not found");
    }
    
    // Test 3: ConstantHighEnergy
    {
        core::SignalData data;
        
        // Add high energy signal everywhere
        for (int i = 0; i < 100; ++i) {
            data.samples.push_back({10.0f, 10.0f});
        }
        
        bool found = preprocessing::SignalDetector::extractActiveRegion(data, 2.0f);
        
        test::check(found, "SignalDetector: Constant high energy found");
        test::check(data.samples.size() > 80, "SignalDetector: Retains high energy signal");
    }
}
