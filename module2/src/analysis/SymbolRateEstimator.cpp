#include "analysis/SymbolRateEstimator.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <complex>

// FFTW3
#include <fftw3.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace module2 {
namespace analysis {

// ──────────────────────────────────────────────────────────────────
// Internal helpers
// ──────────────────────────────────────────────────────────────────

/**
 * @brief Apply Hann window in-place to a real-valued vector.
 */
static void applyHannWindow(std::vector<float>& signal) {
    const size_t N = signal.size();
    if (N <= 1) return;
    for (size_t i = 0; i < N; ++i) {
        float w = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * i / (N - 1)));
        signal[i] *= w;
    }
}

/**
 * @brief Compute one-sided magnitude spectrum of a real signal using FFTW.
 *
 * Returns a vector of length N/2+1 containing |X[k]| for k = 0..N/2.
 * Uses the real-to-complex (r2c) transform for efficiency.
 */
static std::vector<float> computeRealFFTMagnitude(const std::vector<float>& realSignal) {
    const size_t N = realSignal.size();
    const size_t outN = N / 2 + 1;

    float* in = fftwf_alloc_real(N);
    fftwf_complex* out = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * outN);

    std::copy(realSignal.begin(), realSignal.end(), in);

    fftwf_plan plan = fftwf_plan_dft_r2c_1d(static_cast<int>(N), in, out, FFTW_ESTIMATE);
    fftwf_execute(plan);

    std::vector<float> magnitude(outN);
    for (size_t k = 0; k < outN; ++k) {
        float re = out[k][0];
        float im = out[k][1];
        magnitude[k] = std::sqrt(re * re + im * im);
    }

    fftwf_destroy_plan(plan);
    fftwf_free(in);
    fftwf_free(out);

    return magnitude;
}

// ──────────────────────────────────────────────────────────────────
// Peak finder
// ──────────────────────────────────────────────────────────────────

SymbolRateResult SymbolRateEstimator::findPeaks(
    const std::vector<float>& magnitudeSpectrum,
    double sampleRate,
    size_t N,
    float minRate,
    size_t maxCandidates)
{
    SymbolRateResult result;
    result.estimatedRate = 0.0f;
    result.confidence = 0.0f;

    if (magnitudeSpectrum.empty() || N == 0) return result;

    const float freqResolution = static_cast<float>(sampleRate) / N;
    const float effectiveMinRate = std::max(minRate, static_cast<float>(sampleRate * 0.01f));

    // Minimum bin index to search (skip DC and very low frequencies/phase drift)
    size_t minBin = static_cast<size_t>(std::ceil(effectiveMinRate / freqResolution));
    if (minBin < 1) minBin = 1; // Always skip DC (bin 0)

    // Maximum bin: Nyquist (N/2)
    size_t maxBin = magnitudeSpectrum.size();

    if (minBin >= maxBin) return result;

    // Collect all local maxima (bins where mag[k] > mag[k-1] AND mag[k] > mag[k+1])
    struct Peak {
        size_t bin;
        float power;
    };
    std::vector<Peak> peaks;

    for (size_t k = minBin; k < maxBin - 1; ++k) {
        if (magnitudeSpectrum[k] > magnitudeSpectrum[k - 1] &&
            magnitudeSpectrum[k] > magnitudeSpectrum[k + 1]) {
            peaks.push_back({k, magnitudeSpectrum[k]});
        }
    }

    // Sort peaks by power (descending)
    std::sort(peaks.begin(), peaks.end(),
              [](const Peak& a, const Peak& b) { return a.power > b.power; });

    // Take top-N candidates
    size_t numCandidates = std::min(maxCandidates, peaks.size());
    if (numCandidates == 0) {
        // Fallback: find the single maximum bin
        auto it = std::max_element(magnitudeSpectrum.begin() + minBin,
                                    magnitudeSpectrum.begin() + maxBin);
        if (it != magnitudeSpectrum.begin() + maxBin) {
            size_t peakBin = std::distance(magnitudeSpectrum.begin(), it);
            result.estimatedRate = peakBin * freqResolution;
            result.confidence = 0.3f; // Low confidence if no clear local max
            result.candidateRates.push_back(result.estimatedRate);
            result.candidatePowers.push_back(*it);
        }
        return result;
    }

    // Collect significant candidate peaks (power >= 40% of max peak power)
    float maxPower = peaks[0].power;
    float thresholdPower = 0.40f * maxPower;

    size_t bestPeakIndex = 0;
    float lowestFreq = peaks[0].bin * freqResolution;

    for (size_t i = 0; i < numCandidates; ++i) {
        if (peaks[i].power >= thresholdPower) {
            float freq = peaks[i].bin * freqResolution;
            if (freq < lowestFreq) {
                lowestFreq = freq;
                bestPeakIndex = i;
            }
        }
    }

    for (size_t i = 0; i < numCandidates; ++i) {
        result.candidateRates.push_back(peaks[i].bin * freqResolution);
        result.candidatePowers.push_back(peaks[i].power);
    }

    result.estimatedRate = lowestFreq;

    // Harmonic reduction check: If selected peak is an integer harmonic (k * Rs for k in 2..6),
    // check if a fundamental peak exists at f / k with power >= 25% of max power.
    for (int k = 6; k >= 2; --k) {
        size_t fundBin = bestPeakIndex < peaks.size() ? peaks[bestPeakIndex].bin / k : 0;
        if (fundBin >= minBin) {
            float maxFundPower = 0.0f;
            size_t bestFundBin = fundBin;
            for (int offset = -2; offset <= 2; ++offset) {
                int b = static_cast<int>(fundBin) + offset;
                if (b >= static_cast<int>(minBin) && b < static_cast<int>(maxBin)) {
                    if (magnitudeSpectrum[b] > maxFundPower) {
                        maxFundPower = magnitudeSpectrum[b];
                        bestFundBin = static_cast<size_t>(b);
                    }
                }
            }
            if (maxFundPower >= 0.25f * maxPower) {
                result.estimatedRate = bestFundBin * freqResolution;
                break;
            }
        }
    }

    // Confidence: ratio of strongest peak to the next one (peak saliency)
    if (numCandidates >= 2 && result.candidatePowers[1] > 0.0f) {
        float peakRatio = result.candidatePowers[0] / result.candidatePowers[1];
        // Map ratio to confidence: ratio=1 → 0.3, ratio=2 → 0.6, ratio>=4 → 0.95
        result.confidence = std::min(1.0f, 0.3f + 0.65f * (1.0f - 1.0f / peakRatio));
    } else {
        result.confidence = 0.7f; // Only one peak found, moderately confident
    }

    return result;
}

// ──────────────────────────────────────────────────────────────────
// Method 1: Magnitude-Squared FFT
// ──────────────────────────────────────────────────────────────────

SymbolRateResult SymbolRateEstimator::estimateMagnitudeSquared(
    const core::SignalData& data,
    float minRate,
    size_t maxCandidates)
{
    SymbolRateResult result;
    result.estimatedRate = 0.0f;
    result.confidence = 0.0f;

    if (data.samples.size() < 4) return result;

    const size_t N = data.samples.size();

    // Step 1: Compute non-linear delay-and-multiply envelope to expose symbol transitions
    std::vector<float> magSquared(N);
    for (size_t i = 0; i < N; ++i) {
        if (i > 0) {
            std::complex<float> prod = data.samples[i] * std::conj(data.samples[i - 1]);
            magSquared[i] = prod.real();
        } else {
            magSquared[i] = 0.0f;
        }
    }

    // Step 2: Remove DC (subtract mean)
    float mean = std::accumulate(magSquared.begin(), magSquared.end(), 0.0f) / N;
    for (size_t i = 0; i < N; ++i) {
        magSquared[i] -= mean;
    }

    // Step 3: Apply Hann window
    applyHannWindow(magSquared);

    // Step 4: FFT → magnitude spectrum
    std::vector<float> magnitude = computeRealFFTMagnitude(magSquared);

    // Step 5: Peak detection
    return findPeaks(magnitude, data.sampleRate, N, minRate, maxCandidates);
}

// ──────────────────────────────────────────────────────────────────
// Method 2: Delay-and-Multiply
// ──────────────────────────────────────────────────────────────────

SymbolRateResult SymbolRateEstimator::estimateDelayMultiply(
    const core::SignalData& data,
    float minRate,
    size_t maxCandidates)
{
    SymbolRateResult result;
    result.estimatedRate = 0.0f;
    result.confidence = 0.0f;

    if (data.samples.size() < 4) return result;

    const size_t N = data.samples.size();
    const size_t M = N - 1; // delay-and-multiply produces N-1 samples

    // Step 1: Compute d[n] = x[n] * conj(x[n-1])
    // Step 2: Compute r[n] = |d[n]|^2
    std::vector<float> delayMulMagSq(M);
    std::complex<float> prevProd(0, 0);
    for (size_t i = 0; i < M; ++i) {
        std::complex<float> product = data.samples[i + 1] * std::conj(data.samples[i]);
        if (i > 0) {
            delayMulMagSq[i] = std::norm(product - prevProd);
        } else {
            delayMulMagSq[i] = 0.0f;
        }
        prevProd = product;
    }

    // Step 3: Remove DC
    float mean = std::accumulate(delayMulMagSq.begin(), delayMulMagSq.end(), 0.0f) / M;
    for (size_t i = 0; i < M; ++i) {
        delayMulMagSq[i] -= mean;
    }

    // Step 4: Apply Hann window
    applyHannWindow(delayMulMagSq);

    // Step 5: FFT → magnitude spectrum
    std::vector<float> magnitude = computeRealFFTMagnitude(delayMulMagSq);

    // Step 6: Peak detection
    return findPeaks(magnitude, data.sampleRate, M, minRate, maxCandidates);
}

// ──────────────────────────────────────────────────────────────────
// Combined estimator
// ──────────────────────────────────────────────────────────────────

SymbolRateResult SymbolRateEstimator::estimate(
    const core::SignalData& data,
    float minRate)
{
    auto magSqResult = estimateMagnitudeSquared(data, minRate);
    auto delayMulResult = estimateDelayMultiply(data, minRate);

    // If both methods agree (within 5% relative), boost confidence
    if (magSqResult.estimatedRate > 0.0f && delayMulResult.estimatedRate > 0.0f) {
        float relDiff = std::abs(magSqResult.estimatedRate - delayMulResult.estimatedRate)
                        / std::max(magSqResult.estimatedRate, delayMulResult.estimatedRate);

        if (relDiff < 0.05f) {
            // Methods agree — average the two estimates, boost confidence
            SymbolRateResult fused;
            fused.estimatedRate = (magSqResult.estimatedRate + delayMulResult.estimatedRate) / 2.0f;
            fused.confidence = std::min(1.0f,
                (magSqResult.confidence + delayMulResult.confidence) / 2.0f + 0.15f);

            // Merge candidate lists
            fused.candidateRates = magSqResult.candidateRates;
            fused.candidatePowers = magSqResult.candidatePowers;
            for (size_t i = 0; i < delayMulResult.candidateRates.size(); ++i) {
                fused.candidateRates.push_back(delayMulResult.candidateRates[i]);
                fused.candidatePowers.push_back(delayMulResult.candidatePowers[i]);
            }

            return fused;
        }
    }

    // Prefer delay-multiply estimator as it is robust across FSK, PSK, and QAM modulations
    if (delayMulResult.estimatedRate > 0.0f && delayMulResult.confidence >= 0.3f) {
        return delayMulResult;
    }
    return magSqResult;
}

// ──────────────────────────────────────────────────────────────────
// Simple convenience wrapper (backwards compatible)
// ──────────────────────────────────────────────────────────────────

float SymbolRateEstimator::estimateSymbolRate(const core::SignalData& data) {
    if (data.samples.empty()) return 0.0f;
    auto result = estimate(data);
    return result.estimatedRate;
}

} // namespace analysis
} // namespace module2
