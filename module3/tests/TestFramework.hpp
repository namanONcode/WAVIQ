#pragma once
#include <iostream>
#include <string>
#include <cmath>

namespace module3 {
namespace test {

extern int g_checks;
extern int g_failures;

inline void check(bool condition, const std::string& description) {
    g_checks++;
    if (!condition) {
        g_failures++;
        std::cerr << "[FAIL] " << description << "\n";
    }
}

inline void approx_check(float a, float b, const std::string& description, float eps = 1e-3f) {
    g_checks++;
    if (std::abs(a - b) > eps) {
        g_failures++;
        std::cerr << "[FAIL] " << description << " (Expected: " << b << ", Got: " << a << ")\n";
    }
}

#define TEST_CASE(name, func) \
    do { \
        std::cout << "  Running: " << name << "...\n"; \
        func(); \
    } while(0)

} // namespace test
} // namespace module3
