#include "TestFramework.hpp"
#include "features/AmplitudeFeatures.hpp"
#include "features/PhaseFeatures.hpp"
#include "features/FrequencyFeatures.hpp"
#include "features/SpectralFeatures.hpp"
#include "features/CumulantFeatures.hpp"
#include "core/SignalData.hpp"
#include <cmath>
#include <complex>
#include <vector>
#include <iostream>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace module2::features;
using namespace module2::core;
using namespace module2::test;

// ══════════════════════════════════════════════════════════════════
// Signal generators for testing
// ══════════════════════════════════════════════════════════════════

/// Generate a pure complex sinusoid: x[n] = A * exp(j*2*pi*f*n/fs)
static SignalData generateTone(double fs, double freq, float amplitude, size_t N) {
    SignalData data;
    data.sampleRate = fs;
    data.centerFrequency = 0.0;
    data.samples.resize(N);
    for (size_t n = 0; n < N; ++n) {
        float phase = static_cast<float>(2.0 * M_PI * freq * n / fs);
        data.samples[n] = std::complex<float>(
            amplitude * std::cos(phase),
            amplitude * std::sin(phase)
        );
    }
    return data;
}

/// Generate complex white Gaussian noise with given power
static SignalData generateNoise(double fs, float power, size_t N, unsigned seed = 42) {
    SignalData data;
    data.sampleRate = fs;
    data.centerFrequency = 0.0;
    data.samples.resize(N);

    float stddev = std::sqrt(power / 2.0f); // per component
    std::mt19937 rng(seed);
    std::normal_distribution<float> dist(0.0f, stddev);

    for (size_t n = 0; n < N; ++n) {
        data.samples[n] = std::complex<float>(dist(rng), dist(rng));
    }
    return data;
}

/// Generate BPSK signal (constant amplitude, two phase states)
static SignalData generateBPSK(double fs, double symbolRate, size_t numSymbols,
                                unsigned seed = 42) {
    SignalData data;
    data.sampleRate = fs;
    data.centerFrequency = 0.0;

    int sps = static_cast<int>(std::round(fs / symbolRate));
    size_t totalSamples = sps * numSymbols;
    data.samples.resize(totalSamples);

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> bitDist(0, 1);

    for (size_t s = 0; s < numSymbols; ++s) {
        float sym = bitDist(rng) ? 1.0f : -1.0f;
        for (int k = 0; k < sps; ++k) {
            data.samples[s * sps + k] = std::complex<float>(sym, 0.0f);
        }
    }
    return data;
}

/// Generate QPSK signal (constant amplitude, four phase states)
static SignalData generateQPSK(double fs, double symbolRate, size_t numSymbols,
                                unsigned seed = 99) {
    SignalData data;
    data.sampleRate = fs;
    data.centerFrequency = 0.0;

    int sps = static_cast<int>(std::round(fs / symbolRate));
    size_t totalSamples = sps * numSymbols;
    data.samples.resize(totalSamples);

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> symDist(0, 3);

    // QPSK constellation: exp(j*pi/4*(2k+1)) for k=0,1,2,3
    const float inv_sqrt2 = 1.0f / std::sqrt(2.0f);
    std::complex<float> constellation[4] = {
        { inv_sqrt2,  inv_sqrt2},  // pi/4
        {-inv_sqrt2,  inv_sqrt2},  // 3*pi/4
        {-inv_sqrt2, -inv_sqrt2},  // 5*pi/4 = -3*pi/4
        { inv_sqrt2, -inv_sqrt2}   // 7*pi/4 = -pi/4
    };

    for (size_t s = 0; s < numSymbols; ++s) {
        auto sym = constellation[symDist(rng)];
        for (int k = 0; k < sps; ++k) {
            data.samples[s * sps + k] = sym;
        }
    }
    return data;
}

// ══════════════════════════════════════════════════════════════════
// AmplitudeFeatures Tests
// ══════════════════════════════════════════════════════════════════

static void test_amplitude_empty() {
    std::cout << "  [AmplitudeFeatures] test_empty\n";
    SignalData empty;
    empty.sampleRate = 1000.0;
    empty.centerFrequency = 0.0;
    auto stats = AmplitudeFeatures::compute(empty);
    check(stats.mean == 0.0f, "Empty: mean should be 0");
    check(stats.variance == 0.0f, "Empty: variance should be 0");
}

static void test_amplitude_constant() {
    std::cout << "  [AmplitudeFeatures] test_constant_amplitude\n";

    // A pure tone with constant amplitude A=2.0
    auto data = generateTone(10000.0, 1000.0, 2.0f, 4096);
    auto stats = AmplitudeFeatures::compute(data);

    // |x[n]| = A = 2.0 for all n
    approx_check(stats.mean, 2.0f, "Constant amplitude: mean = 2.0", 1e-4f);
    approx_check(stats.variance, 0.0f, "Constant amplitude: variance = 0.0", 1e-4f);
    approx_check(stats.rms, 2.0f, "Constant amplitude: rms = 2.0", 1e-4f);
    approx_check(stats.peak, 2.0f, "Constant amplitude: peak = 2.0", 1e-4f);
    approx_check(stats.crestFactor, 1.0f, "Constant amplitude: crest = 1.0", 1e-4f);
}

static void test_amplitude_noise() {
    std::cout << "  [AmplitudeFeatures] test_noise_statistics\n";

    // Complex Gaussian noise: |x| follows Rayleigh distribution
    // For Rayleigh with sigma^2 per component:
    //   mean = sigma * sqrt(pi/2)
    //   variance = sigma^2 * (4 - pi) / 2
    auto data = generateNoise(10000.0, 2.0f, 100000, 42); // power=2, sigma^2=1 per comp
    auto stats = AmplitudeFeatures::compute(data);

    // sigma = 1.0 per component
    float expectedMean = static_cast<float>(std::sqrt(M_PI / 2.0)); // ~1.2533
    float expectedVar = static_cast<float>((4.0 - M_PI) / 2.0);     // ~0.4292

    approx_check(stats.mean, expectedMean, "Noise: Rayleigh mean", 0.02f);
    approx_check(stats.variance, expectedVar, "Noise: Rayleigh variance", 0.02f);

    // Rayleigh skewness ≈ 0.631
    check(stats.skewness > 0.4f && stats.skewness < 0.9f,
          "Noise: positive skewness (Rayleigh)");
}

static void test_amplitude_envelope() {
    std::cout << "  [AmplitudeFeatures] test_envelope\n";

    auto data = generateTone(10000.0, 500.0, 3.5f, 256);
    auto env = AmplitudeFeatures::envelope(data);

    check(env.size() == 256, "Envelope length matches");
    for (size_t i = 0; i < env.size(); ++i) {
        approx_check(env[i], 3.5f, "Envelope[" + std::to_string(i) + "] = 3.5", 1e-4f);
    }
}

// ══════════════════════════════════════════════════════════════════
// PhaseFeatures Tests
// ══════════════════════════════════════════════════════════════════

static void test_phase_constant() {
    std::cout << "  [PhaseFeatures] test_constant_phase\n";

    // Constant signal x = 1+0j → phase = 0 for all samples
    SignalData data;
    data.sampleRate = 1000.0;
    data.centerFrequency = 0.0;
    data.samples.resize(512, std::complex<float>(1.0f, 0.0f));

    auto stats = PhaseFeatures::compute(data);

    approx_check(stats.mean, 0.0f, "Constant 1+0j: mean phase = 0", 1e-4f);
    approx_check(stats.variance, 0.0f, "Constant 1+0j: phase variance = 0", 1e-4f);
}

static void test_phase_tone() {
    std::cout << "  [PhaseFeatures] test_tone_phase\n";

    // Pure tone: phase advances linearly → after detrending, variance ≈ 0
    auto data = generateTone(10000.0, 1000.0, 1.0f, 4096);
    auto stats = PhaseFeatures::compute(data);

    // Detrended phase should have near-zero variance
    check(stats.variance < 0.01f,
          "Pure tone: detrended phase variance should be near 0");
}

static void test_phase_unwrap() {
    std::cout << "  [PhaseFeatures] test_unwrap\n";

    // Create a linearly increasing phase that wraps
    std::vector<float> wrapped(100);
    for (int i = 0; i < 100; ++i) {
        float raw = static_cast<float>(0.1 * i); // goes past 2*pi
        wrapped[i] = std::fmod(raw + static_cast<float>(M_PI), static_cast<float>(2.0 * M_PI))
                     - static_cast<float>(M_PI);
    }

    auto unwrapped = PhaseFeatures::unwrapPhase(wrapped);

    // Unwrapped should be monotonically increasing (approximately 0.1*i)
    bool monotonic = true;
    for (size_t i = 1; i < unwrapped.size(); ++i) {
        if (unwrapped[i] < unwrapped[i - 1] - 0.01f) {
            monotonic = false;
            break;
        }
    }
    check(monotonic, "Unwrapped phase should be monotonically increasing");

    // Check approximate slope
    float slope = (unwrapped.back() - unwrapped.front()) / 99.0f;
    approx_check(slope, 0.1f, "Unwrapped phase slope ≈ 0.1 rad/sample", 0.02f);
}

static void test_phase_bpsk() {
    std::cout << "  [PhaseFeatures] test_bpsk_phase\n";

    // BPSK has two phase states: 0 and π
    // Detrended phase variance should be substantial
    auto data = generateBPSK(10000.0, 1000.0, 500);
    auto stats = PhaseFeatures::compute(data);

    // BPSK phase variance should be significant (around π^2/4 ≈ 2.47 when symbols flip)
    check(stats.variance > 0.1f,
          "BPSK: phase variance should be non-trivial");
}

// ══════════════════════════════════════════════════════════════════
// FrequencyFeatures Tests
// ══════════════════════════════════════════════════════════════════

static void test_frequency_tone() {
    std::cout << "  [FrequencyFeatures] test_tone_frequency\n";

    double fs = 10000.0;
    double freq = 1500.0;
    auto data = generateTone(fs, freq, 1.0f, 4096);
    auto stats = FrequencyFeatures::compute(data);

    // Mean instantaneous frequency should equal the tone frequency
    approx_check(stats.mean, static_cast<float>(freq),
                 "Tone f=1500Hz: mean inst freq", 1.0f);

    // Very low variance for a pure tone
    check(stats.variance < 10.0f,
          "Tone: frequency variance should be very low");
}

static void test_frequency_vector() {
    std::cout << "  [FrequencyFeatures] test_inst_freq_vector\n";

    double fs = 10000.0;
    double freq = 2000.0;
    auto data = generateTone(fs, freq, 1.0f, 512);
    auto instFreq = FrequencyFeatures::instantaneousFrequency(data);

    check(instFreq.size() == 511, "Inst freq length = N-1");

    // All values should be ≈ 2000 Hz
    for (size_t i = 0; i < instFreq.size(); ++i) {
        approx_check(instFreq[i], 2000.0f,
                     "InstFreq[" + std::to_string(i) + "] = 2000Hz", 1.0f);
    }
}

static void test_frequency_noise() {
    std::cout << "  [FrequencyFeatures] test_noise_frequency\n";

    // White noise: mean frequency ≈ 0, high variance
    auto data = generateNoise(10000.0, 1.0f, 8192);
    auto stats = FrequencyFeatures::compute(data);

    // Mean should be near 0 (symmetric noise)
    check(std::abs(stats.mean) < 500.0f,
          "Noise: mean inst freq should be near 0");

    // Variance should be substantial
    check(stats.variance > 100.0f,
          "Noise: frequency variance should be significant");
}

// ══════════════════════════════════════════════════════════════════
// SpectralFeatures Tests
// ══════════════════════════════════════════════════════════════════

static void test_spectral_empty() {
    std::cout << "  [SpectralFeatures] test_empty\n";

    std::vector<float> psd, freqs;
    auto stats = SpectralFeatures::compute(psd, freqs);
    check(stats.centroid == 0.0f, "Empty PSD: centroid = 0");
    check(stats.entropy == 0.0f, "Empty PSD: entropy = 0");
}

static void test_spectral_single_peak() {
    std::cout << "  [SpectralFeatures] test_single_peak\n";

    // Simulate a PSD with a single strong peak at bin 100 (= 1000 Hz at Fs=10kHz, N=1024)
    size_t N = 1024;
    double fs = 10000.0;
    std::vector<float> psd(N, 0.001f); // noise floor
    std::vector<float> freqBins(N);

    for (size_t k = 0; k < N; ++k) {
        freqBins[k] = static_cast<float>((static_cast<double>(k) / N - 0.5) * fs);
    }

    // Add peak at bin ~614 (which maps to 1000 Hz after centering)
    // freqBins[k] = (k/N - 0.5) * fs; for k=614: (614/1024-0.5)*10000 = 994 Hz
    size_t peakBin = static_cast<size_t>((1000.0 / fs + 0.5) * N);
    psd[peakBin] = 100.0f;

    auto stats = SpectralFeatures::compute(psd, freqBins);

    // Centroid should be near 1000 Hz (dominated by the peak)
    approx_check(stats.centroid, static_cast<float>(freqBins[peakBin]),
                 "Single peak: centroid near 1000Hz", 200.0f);

    // High peak-to-mean ratio
    check(stats.peakToMean > 10.0f, "Single peak: high peak/mean ratio");

    // Low entropy (dominated by single peak)
    check(stats.entropy < 0.9f, "Single peak: low entropy (not uniform)");
}

static void test_spectral_flat() {
    std::cout << "  [SpectralFeatures] test_flat_spectrum\n";

    // Perfectly flat PSD → entropy should be high, flatness ≈ 1
    size_t N = 512;
    std::vector<float> psd(N, 1.0f);
    std::vector<float> freqBins(N);
    for (size_t k = 0; k < N; ++k) {
        freqBins[k] = static_cast<float>(k);
    }

    auto stats = SpectralFeatures::compute(psd, freqBins);

    approx_check(stats.flatness, 1.0f, "Flat PSD: flatness = 1.0", 0.01f);
    approx_check(stats.entropy, 1.0f, "Flat PSD: entropy = 1.0", 0.01f);
    approx_check(stats.peakToMean, 1.0f, "Flat PSD: peak/mean = 1.0", 0.01f);
}

static void test_spectral_rolloff() {
    std::cout << "  [SpectralFeatures] test_rolloff\n";

    // PSD concentrated in first half of bins
    size_t N = 100;
    std::vector<float> psd(N, 0.0f);
    std::vector<float> freqBins(N);
    for (size_t k = 0; k < N; ++k) {
        freqBins[k] = static_cast<float>(k * 100); // 0, 100, 200, ..., 9900 Hz
        if (k < 50) psd[k] = 1.0f; // all energy in first 50 bins
    }

    auto stats = SpectralFeatures::compute(psd, freqBins);

    // 85% rolloff should be around bin 42 (= 4200 Hz) since 85% of 50 = 42.5
    check(stats.rolloff < 5000.0f, "Rolloff: should be in first half");
    check(stats.rolloff > 3000.0f, "Rolloff: should be near 85% of energy band");
}

// ══════════════════════════════════════════════════════════════════
// CumulantFeatures Tests
// ══════════════════════════════════════════════════════════════════

static void test_cumulant_empty() {
    std::cout << "  [CumulantFeatures] test_empty\n";

    SignalData tiny;
    tiny.sampleRate = 1000.0;
    tiny.centerFrequency = 0.0;
    tiny.samples.resize(4, std::complex<float>(1.0f, 0.0f)); // < 16

    auto cv = CumulantFeatures::compute(tiny);
    check(cv.C21 == 0.0f, "Too-short signal: C21 = 0");
}

static void test_cumulant_bpsk() {
    std::cout << "  [CumulantFeatures] test_bpsk_cumulants\n";

    // BPSK: theoretical normalized cumulants:
    //   C40 = -2, C42 = -2
    // (After unit-power normalization)
    auto data = generateBPSK(10000.0, 1000.0, 10000, 42);
    auto cv = CumulantFeatures::compute(data);

    // C21 should be ≈ 1.0 after normalization
    approx_check(cv.C21, 1.0f, "BPSK: C21 ≈ 1.0 (unit power)", 0.1f);

    // For BPSK: |C40| ≈ 2.0, |C42| ≈ 2.0
    approx_check(cv.C40, 2.0f, "BPSK: |C40| ≈ 2.0", 0.3f);
    approx_check(cv.C42, -2.0f, "BPSK: C42 ≈ -2.0", 0.3f);

    std::cout << "    C20_mag=" << cv.C20_mag << " C21=" << cv.C21
              << " C40=" << cv.C40 << " C42=" << cv.C42
              << " C60=" << cv.C60 << "\n";
}

static void test_cumulant_qpsk() {
    std::cout << "  [CumulantFeatures] test_qpsk_cumulants\n";

    // QPSK: theoretical normalized cumulants:
    //   C40 = 1, C42 = -1
    auto data = generateQPSK(10000.0, 1000.0, 10000, 99);
    auto cv = CumulantFeatures::compute(data);

    approx_check(cv.C21, 1.0f, "QPSK: C21 ≈ 1.0", 0.1f);

    // QPSK: |C40| ≈ 1.0, C42 ≈ -1.0
    approx_check(cv.C40, 1.0f, "QPSK: |C40| ≈ 1.0", 0.3f);
    approx_check(cv.C42, -1.0f, "QPSK: C42 ≈ -1.0", 0.3f);

    std::cout << "    C20_mag=" << cv.C20_mag << " C21=" << cv.C21
              << " C40=" << cv.C40 << " C42=" << cv.C42
              << " C60=" << cv.C60 << "\n";
}

static void test_cumulant_noise() {
    std::cout << "  [CumulantFeatures] test_noise_cumulants\n";

    // Gaussian noise: all cumulants above 2nd order should be ≈ 0
    auto data = generateNoise(10000.0, 1.0f, 50000, 42);
    auto cv = CumulantFeatures::compute(data);

    approx_check(cv.C21, 1.0f, "Noise: C21 ≈ 1.0", 0.1f);

    // C40 and C42 should be near 0 for Gaussian
    check(std::abs(cv.C40) < 0.3f, "Noise: |C40| ≈ 0 (Gaussian)");
    check(std::abs(cv.C42) < 0.3f, "Noise: |C42| ≈ 0 (Gaussian)");

    std::cout << "    C20_mag=" << cv.C20_mag << " C21=" << cv.C21
              << " C40=" << cv.C40 << " C42=" << cv.C42 << "\n";
}

static void test_cumulant_bpsk_vs_qpsk_discrimination() {
    std::cout << "  [CumulantFeatures] test_bpsk_vs_qpsk_discrimination\n";

    auto bpsk = generateBPSK(10000.0, 1000.0, 10000, 42);
    auto qpsk = generateQPSK(10000.0, 1000.0, 10000, 99);

    auto cvBPSK = CumulantFeatures::compute(bpsk);
    auto cvQPSK = CumulantFeatures::compute(qpsk);

    // Key discrimination: |C40_BPSK| > |C40_QPSK| and |C42_BPSK| > |C42_QPSK|
    check(cvBPSK.C40 > cvQPSK.C40 + 0.3f,
          "Discrimination: |C40_BPSK| > |C40_QPSK|");

    // Both C42 are negative, but |C42_BPSK| > |C42_QPSK|
    check(std::abs(cvBPSK.C42) > std::abs(cvQPSK.C42) + 0.3f,
          "Discrimination: |C42_BPSK| > |C42_QPSK|");
}

static void test_cumulant_toVector() {
    std::cout << "  [CumulantFeatures] test_toVector\n";

    CumulantValues cv = {};
    cv.C20_mag = 1.0f; cv.C21 = 2.0f;
    cv.C40 = 3.0f; cv.C41 = 4.0f; cv.C42 = 5.0f;
    cv.C60 = 6.0f; cv.C61 = 7.0f; cv.C62 = 8.0f; cv.C63 = 9.0f;

    auto vec = CumulantFeatures::toVector(cv);

    check(vec.size() == 9, "toVector: length = 9");
    approx_check(vec[0], 1.0f, "toVector[0] = C20_mag", 1e-6f);
    approx_check(vec[4], 5.0f, "toVector[4] = C42", 1e-6f);
    approx_check(vec[8], 9.0f, "toVector[8] = C63", 1e-6f);
}

// ══════════════════════════════════════════════════════════════════
// Public entry point
// ══════════════════════════════════════════════════════════════════

void run_features_tests() {
    std::cout << "[Suite] Feature Extraction Tests\n";

    // Amplitude
    std::cout << " -- AmplitudeFeatures --\n";
    test_amplitude_empty();
    test_amplitude_constant();
    test_amplitude_noise();
    test_amplitude_envelope();

    // Phase
    std::cout << " -- PhaseFeatures --\n";
    test_phase_constant();
    test_phase_tone();
    test_phase_unwrap();
    test_phase_bpsk();

    // Frequency
    std::cout << " -- FrequencyFeatures --\n";
    test_frequency_tone();
    test_frequency_vector();
    test_frequency_noise();

    // Spectral
    std::cout << " -- SpectralFeatures --\n";
    test_spectral_empty();
    test_spectral_single_peak();
    test_spectral_flat();
    test_spectral_rolloff();

    // Cumulants
    std::cout << " -- CumulantFeatures --\n";
    test_cumulant_empty();
    test_cumulant_bpsk();
    test_cumulant_qpsk();
    test_cumulant_noise();
    test_cumulant_bpsk_vs_qpsk_discrimination();
    test_cumulant_toVector();
}
