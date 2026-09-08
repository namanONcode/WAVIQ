#include "features/CumulantFeatures.hpp"
#include <cmath>
#include <complex>
#include <numeric>

namespace module2 {
namespace features {

CumulantValues CumulantFeatures::compute(const core::SignalData& data) {
    CumulantValues cv = {};

    if (data.samples.size() < 16) return cv;

    const size_t N = data.samples.size();

    // Step 1: Compute signal power and normalize to unit power
    // C21 = E[|x|^2] — this is the signal power
    double powerSum = 0.0;
    for (size_t i = 0; i < N; ++i) {
        float re = data.samples[i].real();
        float im = data.samples[i].imag();
        powerSum += static_cast<double>(re) * re + static_cast<double>(im) * im;
    }
    double signalPower = powerSum / N;

    if (signalPower < 1e-20) return cv; // Signal is effectively zero

    // Normalize to unit power
    float normFactor = static_cast<float>(1.0 / std::sqrt(signalPower));
    std::vector<std::complex<float>> xn(N);
    for (size_t i = 0; i < N; ++i) {
        xn[i] = data.samples[i] * normFactor;
    }

    // Step 2: Remove DC (mean subtraction on normalized signal)
    std::complex<double> dcSum(0.0, 0.0);
    for (size_t i = 0; i < N; ++i) {
        dcSum += std::complex<double>(xn[i].real(), xn[i].imag());
    }
    std::complex<float> dc(static_cast<float>(dcSum.real() / N),
                            static_cast<float>(dcSum.imag() / N));
    for (size_t i = 0; i < N; ++i) {
        xn[i] -= dc;
    }

    // Step 3: Compute moments
    // M_pq = E[x^(p-q) * (x*)^q]
    // We need: E[x^2], E[|x|^2], E[x^3*x*], E[|x|^4], E[x^4]
    // And 6th-order: E[x^6], E[x^5*x*], E[x^4*(x*)^2], E[x^3*(x*)^3]

    std::complex<double> M20(0,0); // E[x^2]
    double M21 = 0.0;              // E[|x|^2]  (always real)
    std::complex<double> M40(0,0); // E[x^4]
    std::complex<double> M41(0,0); // E[x^3 * x*]
    double M42 = 0.0;              // E[|x|^4]  (always real)

    std::complex<double> M60(0,0); // E[x^6]
    std::complex<double> M61(0,0); // E[x^5 * x*]
    std::complex<double> M62(0,0); // E[x^4 * (x*)^2]
    double M63 = 0.0;              // E[|x|^6]  (= E[x^3 * (x*)^3]) always real

    for (size_t i = 0; i < N; ++i) {
        std::complex<double> x(xn[i].real(), xn[i].imag());
        std::complex<double> xc = std::conj(x);

        std::complex<double> x2 = x * x;
        double ax2 = std::norm(x); // |x|^2
        std::complex<double> x3 = x2 * x;
        std::complex<double> x4 = x3 * x;

        M20 += x2;
        M21 += ax2;
        M40 += x4;
        M41 += x3 * xc;
        M42 += ax2 * ax2;

        std::complex<double> x5 = x4 * x;
        std::complex<double> x6 = x5 * x;

        M60 += x6;
        M61 += x5 * xc;
        M62 += x4 * xc * xc;
        M63 += ax2 * ax2 * ax2;
    }

    M20 /= static_cast<double>(N);
    M21 /= static_cast<double>(N);
    M40 /= static_cast<double>(N);
    M41 /= static_cast<double>(N);
    M42 /= static_cast<double>(N);
    M60 /= static_cast<double>(N);
    M61 /= static_cast<double>(N);
    M62 /= static_cast<double>(N);
    M63 /= static_cast<double>(N);

    // Step 4: Compute cumulants from moments
    // 2nd order:
    // C20 = M20    (= E[x^2])
    // C21 = M21    (= E[|x|^2])  — should be ≈ 1 after normalization

    cv.C20_mag = static_cast<float>(std::abs(M20));
    cv.C21 = static_cast<float>(M21);

    // 4th order (Cum → Moment formulas):
    // C40 = M40 - 3 * M20^2
    // C41 = M41 - 3 * M20 * M21  (M21 is real)
    // C42 = M42 - |M20|^2 - 2 * M21^2

    std::complex<double> C40c = M40 - 3.0 * M20 * M20;
    std::complex<double> C41c = M41 - 3.0 * M20 * M21;
    double C42d = M42 - std::norm(M20) - 2.0 * M21 * M21;

    cv.C40 = static_cast<float>(std::abs(C40c));
    cv.C41 = static_cast<float>(std::abs(C41c));
    cv.C42 = static_cast<float>(C42d);

    // 6th order (Cum → Moment formulas):
    // C60 = M60 - 15*M20*M40 + 30*M20^3
    // C61 = M61 - 5*M41*M20 - 10*M21*M40 + 30*M20^2*M21
    // C62 = M62 - |M20|^2*M42/M21 ... (simplified)
    //   Actually, exact formulas:
    // C60 = M60 - 15*M40*M20 + 30*M20^3
    // C61 = M61 - 5*M41*M20 - 10*M40*M21 + 30*M20^2*M21
    // C62 = M62 - 6*M42*M20 - 8*M41*conj(M20) - M40*conj(M20)^2
    //       + 6*M20^2*conj(M20)^2 + 24*M21^2*M20
    // C63 = M63 - 9*M42*M21 + 12*M21^3 - 3*|M40|^2/... (simplified)
    //   Use the standard formulas:
    // C63 = M63 - 9*M42*M21 - 12*M21^3 + 18*M21*(|M20|^2 + 2*M21^2)  — careful

    // Standard 6th-order cumulant formulas (from moments, for zero-mean):
    std::complex<double> C60c = M60 - 15.0 * M40 * M20 + 30.0 * M20 * M20 * M20;
    std::complex<double> C61c = M61 - 5.0 * M41 * M20 - 10.0 * M40 * M21 
                                 + 30.0 * M20 * M20 * M21;
    std::complex<double> C62c = M62 - 6.0 * M42 * M20 
                                 - 8.0 * M41 * std::conj(M20) 
                                 - M40 * std::conj(M20) * std::conj(M20)
                                 + 6.0 * M20 * M20 * std::conj(M20) * std::conj(M20)
                                 + 24.0 * M21 * M21 * M20;
    double C63d = M63 - 9.0 * M42 * M21
                  + 12.0 * M21 * M21 * M21
                  - 3.0 * std::norm(M40)
                  - 12.0 * std::norm(M20) * M21;

    cv.C60 = static_cast<float>(std::abs(C60c));
    cv.C61 = static_cast<float>(std::abs(C61c));
    cv.C62 = static_cast<float>(std::abs(C62c));
    cv.C63 = static_cast<float>(std::abs(C63d));

    return cv;
}

std::vector<float> CumulantFeatures::toVector(const CumulantValues& cv) {
    return {cv.C20_mag, cv.C21, cv.C40, cv.C41, cv.C42,
            cv.C60, cv.C61, cv.C62, cv.C63};
}

} // namespace features
} // namespace module2
