#include "TestFramework.hpp"
#include <iostream>

namespace module3 {
namespace test {
int g_checks = 0;
int g_failures = 0;
}
}

void run_deinterleaver_tests();
void run_fec_tests();
void run_correlator_tests();

int main() {
    std::cout << "Running Module 3 tests...\n";
    
    run_deinterleaver_tests();
    run_fec_tests();
    run_correlator_tests();

    std::cout << module3::test::g_checks << "/" << module3::test::g_checks << " checks passed (" 
              << (module3::test::g_failures == 0 ? "100% pass rate, 0 failures" : std::to_string(module3::test::g_failures) + " failures") 
              << ")\n";

    return module3::test::g_failures == 0 ? 0 : 1;
}
