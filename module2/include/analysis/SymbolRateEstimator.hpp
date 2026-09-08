#pragma once
#include "core/SignalData.hpp"
#include <vector>
#include <cstddef>

namespace module2 {
namespace analysis {

/**
 * @brief Result of symbol rate estimation, including candidate rates and confidence.
 */
struct SymbolRateResult {
    float estimatedRate;       ///< Best estimate of symbol rate (Hz)
    float confidence;          ///< Confidence score [0.0, 1.0]
    std::vector<float> candidateRates;  ///< Top candidate peaks (Hz)
    std::vector<float> candidatePowers; ///< Corresponding peak powers (normalized)
};

/**
 * @brief Estimates the symbol rate (baud rate) of a digitally modulated signal.
 *
 * Implements two complementary non-linear methods:
 *
 * 1. **Magnitude-Squared FFT**: Computes |x[n]|^2, removes DC, applies Hann window,
 *    takes FFT and finds the strongest spectral peak. The peak frequency corresponds
 *    to the symbol rate Rs for many modulation types (PSK, QAM, FSK).
 *
 * 2. **Delay-and-Multiply**: Computes x[n] * conj(x[n-1]), which produces a signal
 *    whose spectrum has peaks at multiples of the symbol rate. This is more robust
 *    to carrier offset than pure squaring.
 *
 * Both methods detect cyclostationary features induced by the periodic structure
 * of the transmitted symbol stream.
 */
class SymbolRateEstimator {
public:
    /**
     * @brief Estimates the symbol rate using magnitude-squared spectral analysis.
     *
     * Algorithm:
     * 1. Compute r[n] = |x[n]|^2
     * 2. Remove DC component (mean subtraction)
     * 3. Apply Hann window
     * 4. Compute FFT of r[n]
     * 5. Compute magnitude spectrum
     * 6. Find dominant peak in [minRate, Fs/2] range
     *
     * @param data       Input signal data (complex IQ samples + sample rate).
     * @param minRate    Minimum expected symbol rate in Hz (default: 100 Hz).
     * @param maxCandidates Maximum number of candidate peaks to return (default: 5).
     * @return SymbolRateResult with best estimate and candidates.
     */
    static SymbolRateResult estimateMagnitudeSquared(
        const core::SignalData& data,
        float minRate = 100.0f,
        size_t maxCandidates = 5);

    /**
     * @brief Estimates the symbol rate using delay-and-multiply spectral analysis.
     *
     * Algorithm:
     * 1. Compute d[n] = x[n] * conj(x[n-1])
     * 2. Compute r[n] = |d[n]|^2
     * 3. Remove DC, apply Hann window
     * 4. FFT and peak detection
     *
     * @param data       Input signal data.
     * @param minRate    Minimum expected symbol rate in Hz (default: 100 Hz).
     * @param maxCandidates Maximum number of candidate peaks to return (default: 5).
     * @return SymbolRateResult with best estimate and candidates.
     */
    static SymbolRateResult estimateDelayMultiply(
        const core::SignalData& data,
        float minRate = 100.0f,
        size_t maxCandidates = 5);

    /**
     * @brief Combined estimator: runs both methods, cross-validates, returns best result.
     *
     * @param data       Input signal data.
     * @param minRate    Minimum expected symbol rate in Hz (default: 100 Hz).
     * @return SymbolRateResult with the best fused estimate.
     */
    static SymbolRateResult estimate(
        const core::SignalData& data,
        float minRate = 100.0f);

    /**
     * @brief Simple convenience wrapper — returns just the estimated symbol rate in Hz.
     * Calls estimate() internally.
     *
     * @param data Input signal data.
     * @return Estimated symbol rate in Hz, or 0.0f if estimation fails.
     */
    static float estimateSymbolRate(const core::SignalData& data);

private:
    /**
     * @brief Finds peaks in a magnitude spectrum, excluding the DC region.
     *
     * @param magnitudeSpectrum The magnitude spectrum (one-sided, length N/2+1).
     * @param sampleRate        The sample rate of the *original* signal being analyzed.
     * @param N                 The FFT length.
     * @param minRate           Minimum frequency to consider.
     * @param maxCandidates     Maximum number of peaks.
     * @return SymbolRateResult populated with peaks.
     */
    static SymbolRateResult findPeaks(
        const std::vector<float>& magnitudeSpectrum,
        double sampleRate,
        size_t N,
        float minRate,
        size_t maxCandidates);
};

} // namespace analysis
} // namespace module2
