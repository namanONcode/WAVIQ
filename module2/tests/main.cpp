#include "TestFramework.hpp"
#include <iostream>

namespace module2 {
namespace test {
int g_checks = 0;
int g_failures = 0;
}
}

// Forward declarations of test suites
void run_dcremoval_tests();
void run_normalizer_tests();
void run_filter_tests();
void run_signaldetector_tests();
void run_symbolrateestimator_tests();

int main() {
    std::cout << "Running Module 2 tests...\n";
    
    run_dcremoval_tests();
    run_normalizer_tests();
    run_filter_tests();
    run_signaldetector_tests();
    run_symbolrateestimator_tests();

    std::cout << module2::test::g_checks << "/" << module2::test::g_checks << " checks passed (" 
              << (module2::test::g_failures == 0 ? "100% pass rate, 0 failures" : std::to_string(module2::test::g_failures) + " failures") 
              << ")\n";

    return module2::test::g_failures == 0 ? 0 : 1;
}
