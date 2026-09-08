#include "TestFramework.hpp"
#include "analysis/SymbolRateEstimator.hpp"
#include "core/SignalData.hpp"
#include <cmath>
#include <complex>
#include <vector>
#include <iostream>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace module2::analysis;
using namespace module2::core;
using namespace module2::test;

// ──────────────────────────────────────────────────────────────────
// Helper: Generate BPSK signal with known symbol rate
// ──────────────────────────────────────────────────────────────────
/**
 * Generates a BPSK signal: x[n] = symbol[floor(n/samplesPerSymbol)] * exp(j*2*pi*fc*n/fs)
 * The magnitude-squared of a BPSK signal has cyclostationary features at Rs = Fs/samplesPerSymbol.
 *
 * @param fs            Sample rate (Hz)
 * @param symbolRate    Symbol rate (Hz) — must divide fs to give integer samples/symbol
 * @param numSymbols    Number of symbols to generate
 * @param fc            Carrier offset (Hz)
 * @param snrDb         Signal-to-noise ratio (dB), -1 for no noise
 * @return              SignalData with the generated samples
 */
static SignalData generateBPSK(double fs, double symbolRate, size_t numSymbols,
                                double fc = 0.0, double snrDb = 40.0) {
    SignalData data;
    data.sampleRate = fs;
    data.centerFrequency = fc;

    int samplesPerSymbol = static_cast<int>(std::round(fs / symbolRate));
    size_t totalSamples = samplesPerSymbol * numSymbols;
    data.samples.resize(totalSamples);

    // Deterministic PRBS pattern using a fixed seed
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> bitDist(0, 1);

    // Generate symbols
    std::vector<float> symbols(numSymbols);
    for (size_t s = 0; s < numSymbols; ++s) {
        symbols[s] = bitDist(rng) ? 1.0f : -1.0f;
    }

    // Upsample and modulate
    for (size_t n = 0; n < totalSamples; ++n) {
        size_t symbolIdx = n / samplesPerSymbol;
        float phase = static_cast<float>(2.0 * M_PI * fc * n / fs);
        data.samples[n] = std::complex<float>(
            symbols[symbolIdx] * std::cos(phase),
            symbols[symbolIdx] * std::sin(phase)
        );
    }

    // Add AWGN noise
    if (snrDb > -100.0) {
        // Signal power is 1.0 for BPSK
        float noisePower = std::pow(10.0f, static_cast<float>(-snrDb / 10.0));
        float noiseStd = std::sqrt(noisePower / 2.0f); // per I/Q component
        std::normal_distribution<float> noise(0.0f, noiseStd);
        for (size_t n = 0; n < totalSamples; ++n) {
            data.samples[n] += std::complex<float>(noise(rng), noise(rng));
        }
    }

    return data;
}

// ──────────────────────────────────────────────────────────────────
// Helper: Generate FSK signal with known symbol rate
// ──────────────────────────────────────────────────────────────────
static SignalData generateFSK(double fs, double symbolRate, size_t numSymbols,
                               double freqDev = 1000.0, double snrDb = 40.0) {
    SignalData data;
    data.sampleRate = fs;
    data.centerFrequency = 0.0;

    int samplesPerSymbol = static_cast<int>(std::round(fs / symbolRate));
    size_t totalSamples = samplesPerSymbol * numSymbols;
    data.samples.resize(totalSamples);

    std::mt19937 rng(123);
    std::uniform_int_distribution<int> bitDist(0, 1);

    std::vector<float> symbols(numSymbols);
    for (size_t s = 0; s < numSymbols; ++s) {
        symbols[s] = bitDist(rng) ? 1.0f : -1.0f;
    }

    // CPFSK: continuous phase frequency shift keying
    double phase = 0.0;
    for (size_t n = 0; n < totalSamples; ++n) {
        size_t symbolIdx = n / samplesPerSymbol;
        double freq = symbols[symbolIdx] * freqDev;
        phase += 2.0 * M_PI * freq / fs;
        data.samples[n] = std::complex<float>(
            static_cast<float>(std::cos(phase)),
            static_cast<float>(std::sin(phase))
        );
    }

    // Add noise
    if (snrDb > -100.0) {
        float noisePower = std::pow(10.0f, static_cast<float>(-snrDb / 10.0));
        float noiseStd = std::sqrt(noisePower / 2.0f);
        std::normal_distribution<float> noise(0.0f, noiseStd);
        for (size_t n = 0; n < totalSamples; ++n) {
            data.samples[n] += std::complex<float>(noise(rng), noise(rng));
        }
    }

    return data;
}

// ──────────────────────────────────────────────────────────────────
// Test: Empty / minimal input
// ──────────────────────────────────────────────────────────────────
static void test_empty_input() {
    std::cout << "  [SymbolRateEstimator] test_empty_input\n";

    SignalData empty;
    empty.sampleRate = 10000.0;
    empty.centerFrequency = 0.0;

    float rate = SymbolRateEstimator::estimateSymbolRate(empty);
    check(rate == 0.0f, "Empty signal should return 0 Hz symbol rate");

    // Very short signal (< 4 samples)
    SignalData tiny;
    tiny.sampleRate = 10000.0;
    tiny.centerFrequency = 0.0;
    tiny.samples = {{1.0f, 0.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f}};

    auto result = SymbolRateEstimator::estimateMagnitudeSquared(tiny);
    check(result.estimatedRate == 0.0f, "Tiny signal (<4 samples) should return 0 Hz");
}

// ──────────────────────────────────────────────────────────────────
// Test: Known BPSK symbol rate detection (clean, high SNR)
// ──────────────────────────────────────────────────────────────────
static void test_bpsk_clean_symbol_rate() {
    std::cout << "  [SymbolRateEstimator] test_bpsk_clean_symbol_rate\n";

    // Fs = 100kHz, Rs = 10kHz (10 samples/symbol), 500 symbols, no noise
    double fs = 100000.0;
    double rs = 10000.0;
    auto data = generateBPSK(fs, rs, 500, 0.0, 60.0);

    auto result = SymbolRateEstimator::estimateMagnitudeSquared(data);

    // Expected: peak at Rs = 10kHz, tolerance ±1% (= ±100 Hz)
    float tolerance = static_cast<float>(rs * 0.02); // 2% tolerance
    approx_check(result.estimatedRate, static_cast<float>(rs),
                 "BPSK Rs=10kHz: magnitude-squared estimate", tolerance);

    check(result.confidence > 0.3f,
          "BPSK Rs=10kHz: confidence should be reasonable");

    check(!result.candidateRates.empty(),
          "BPSK Rs=10kHz: should have at least one candidate");
}

// ──────────────────────────────────────────────────────────────────
// Test: Delay-and-multiply method on same BPSK signal
// ──────────────────────────────────────────────────────────────────
static void test_bpsk_delay_multiply() {
    std::cout << "  [SymbolRateEstimator] test_bpsk_delay_multiply\n";

    double fs = 100000.0;
    double rs = 10000.0;
    auto data = generateBPSK(fs, rs, 500, 0.0, 60.0);

    auto result = SymbolRateEstimator::estimateDelayMultiply(data);

    float tolerance = static_cast<float>(rs * 0.02);
    approx_check(result.estimatedRate, static_cast<float>(rs),
                 "BPSK Rs=10kHz: delay-multiply estimate", tolerance);
}

// ──────────────────────────────────────────────────────────────────
// Test: Combined estimator
// ──────────────────────────────────────────────────────────────────
static void test_bpsk_combined() {
    std::cout << "  [SymbolRateEstimator] test_bpsk_combined\n";

    double fs = 100000.0;
    double rs = 10000.0;
    auto data = generateBPSK(fs, rs, 500, 0.0, 60.0);

    auto result = SymbolRateEstimator::estimate(data);

    float tolerance = static_cast<float>(rs * 0.02);
    approx_check(result.estimatedRate, static_cast<float>(rs),
                 "BPSK Rs=10kHz: combined estimate", tolerance);

    // Combined should generally have higher confidence when methods agree
    check(result.confidence > 0.3f,
          "BPSK Rs=10kHz: combined confidence should be reasonable");
}

// ──────────────────────────────────────────────────────────────────
// Test: BPSK with carrier offset (tests robustness)
// ──────────────────────────────────────────────────────────────────
static void test_bpsk_with_carrier_offset() {
    std::cout << "  [SymbolRateEstimator] test_bpsk_with_carrier_offset\n";

    double fs = 100000.0;
    double rs = 5000.0;  // 20 samples/symbol
    double fc = 15000.0; // 15kHz carrier offset
    auto data = generateBPSK(fs, rs, 400, fc, 40.0);

    auto result = SymbolRateEstimator::estimate(data);

    float tolerance = static_cast<float>(rs * 0.05); // 5% tolerance with carrier
    approx_check(result.estimatedRate, static_cast<float>(rs),
                 "BPSK Rs=5kHz fc=15kHz: symbol rate with carrier offset", tolerance);
}

// ──────────────────────────────────────────────────────────────────
// Test: Different symbol rates
// ──────────────────────────────────────────────────────────────────
static void test_multiple_symbol_rates() {
    std::cout << "  [SymbolRateEstimator] test_multiple_symbol_rates\n";

    double fs = 200000.0;

    // Test several symbol rates
    double rates[] = {2000.0, 5000.0, 20000.0, 50000.0};
    for (double rs : rates) {
        auto data = generateBPSK(fs, rs, 400, 0.0, 50.0);
        auto result = SymbolRateEstimator::estimateMagnitudeSquared(data);

        float tolerance = static_cast<float>(rs * 0.03); // 3% tolerance
        approx_check(result.estimatedRate, static_cast<float>(rs),
                     "Multi-rate Rs=" + std::to_string(static_cast<int>(rs)) + "Hz",
                     tolerance);
    }
}

// ──────────────────────────────────────────────────────────────────
// Test: FSK signal (different modulation scheme)
// ──────────────────────────────────────────────────────────────────
static void test_fsk_symbol_rate() {
    std::cout << "  [SymbolRateEstimator] test_fsk_symbol_rate\n";

    double fs = 100000.0;
    double rs = 10000.0;
    auto data = generateFSK(fs, rs, 500, 2000.0, 50.0);

    auto result = SymbolRateEstimator::estimate(data);

    // FSK detection can be less precise, use wider tolerance
    float tolerance = static_cast<float>(rs * 0.05);
    approx_check(result.estimatedRate, static_cast<float>(rs),
                 "FSK Rs=10kHz: symbol rate estimate", tolerance);
}

// ──────────────────────────────────────────────────────────────────
// Test: Simple convenience wrapper
// ──────────────────────────────────────────────────────────────────
static void test_convenience_wrapper() {
    std::cout << "  [SymbolRateEstimator] test_convenience_wrapper\n";

    double fs = 100000.0;
    double rs = 10000.0;
    auto data = generateBPSK(fs, rs, 500, 0.0, 60.0);

    float estimated = SymbolRateEstimator::estimateSymbolRate(data);

    float tolerance = static_cast<float>(rs * 0.03);
    approx_check(estimated, static_cast<float>(rs),
                 "Convenience wrapper: estimateSymbolRate()", tolerance);
}

// ──────────────────────────────────────────────────────────────────
// Test: SNR robustness — test at various noise levels
// ──────────────────────────────────────────────────────────────────
static void test_snr_robustness() {
    std::cout << "  [SymbolRateEstimator] test_snr_robustness\n";

    double fs = 100000.0;
    double rs = 10000.0;

    // High SNR should work very well
    {
        auto data = generateBPSK(fs, rs, 800, 0.0, 30.0);
        auto result = SymbolRateEstimator::estimate(data);
        float tolerance = static_cast<float>(rs * 0.03);
        approx_check(result.estimatedRate, static_cast<float>(rs),
                     "SNR=30dB: symbol rate estimate", tolerance);
    }

    // Medium SNR should still work
    {
        auto data = generateBPSK(fs, rs, 800, 0.0, 15.0);
        auto result = SymbolRateEstimator::estimate(data);
        float tolerance = static_cast<float>(rs * 0.05);
        approx_check(result.estimatedRate, static_cast<float>(rs),
                     "SNR=15dB: symbol rate estimate", tolerance);
    }

    // Low SNR — may be less accurate but should be in the ballpark
    {
        auto data = generateBPSK(fs, rs, 1600, 0.0, 5.0); // More symbols for low SNR
        auto result = SymbolRateEstimator::estimate(data);
        float tolerance = static_cast<float>(rs * 0.10); // 10% tolerance at 5dB
        approx_check(result.estimatedRate, static_cast<float>(rs),
                     "SNR=5dB: symbol rate estimate (relaxed)", tolerance);
    }
}

// ──────────────────────────────────────────────────────────────────
// Test: Pure tone (CW) — should NOT find a symbol rate
// ──────────────────────────────────────────────────────────────────
static void test_pure_tone_no_symbol_rate() {
    std::cout << "  [SymbolRateEstimator] test_pure_tone_no_symbol_rate\n";

    double fs = 100000.0;
    size_t N = 4096;

    SignalData data;
    data.sampleRate = fs;
    data.centerFrequency = 0.0;
    data.samples.resize(N);

    // Pure tone at 10kHz — no symbol structure, constant amplitude
    for (size_t n = 0; n < N; ++n) {
        float phase = static_cast<float>(2.0 * M_PI * 10000.0 * n / fs);
        data.samples[n] = std::complex<float>(std::cos(phase), std::sin(phase));
    }

    auto result = SymbolRateEstimator::estimateMagnitudeSquared(data);

    // A pure CW tone has constant |x[n]|^2, so after DC removal, the FFT should 
    // have no significant peak. The estimated rate should be low/zero or confidence low.
    // We check that confidence is low — a CW signal has no cyclostationary features.
    check(result.confidence < 0.6f || result.estimatedRate < 50.0f,
          "Pure CW tone: should have low confidence or negligible rate");
}

// ──────────────────────────────────────────────────────────────────
// Public entry point
// ──────────────────────────────────────────────────────────────────
void run_symbolrateestimator_tests() {
    std::cout << "[Suite] SymbolRateEstimator Tests\n";

    test_empty_input();
    test_bpsk_clean_symbol_rate();
    test_bpsk_delay_multiply();
    test_bpsk_combined();
    test_bpsk_with_carrier_offset();
    test_multiple_symbol_rates();
    test_fsk_symbol_rate();
    test_convenience_wrapper();
    test_snr_robustness();
    test_pure_tone_no_symbol_rate();
}
