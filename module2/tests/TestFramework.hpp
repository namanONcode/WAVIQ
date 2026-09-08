#pragma once
#include <iostream>
#include <string>
#include <cmath>
#include <complex>

namespace module2 {
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

inline bool approx(float a, float b, float eps = 1e-3f) {
    return std::abs(a - b) <= eps;
}

inline bool approx(std::complex<float> a, std::complex<float> b, float eps = 1e-3f) {
    return std::abs(a.real() - b.real()) <= eps && std::abs(a.imag() - b.imag()) <= eps;
}

inline void approx_check(float a, float b, const std::string& description, float eps = 1e-3f) {
    g_checks++;
    if (!approx(a, b, eps)) {
        g_failures++;
        std::cerr << "[FAIL] " << description << " (Expected: " << b << ", Got: " << a << ")\n";
    }
}

inline void approx_check(std::complex<float> a, std::complex<float> b, const std::string& description, float eps = 1e-3f) {
    g_checks++;
    if (!approx(a, b, eps)) {
        g_failures++;
        std::cerr << "[FAIL] " << description << " (Expected: " << b.real() << "+" << b.imag() << "i, Got: " << a.real() << "+" << a.imag() << "i)\n";
    }
}

} // namespace test
} // namespace module2
