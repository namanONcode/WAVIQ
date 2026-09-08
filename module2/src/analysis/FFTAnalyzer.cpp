#include "analysis/FFTAnalyzer.hpp"
#include <cmath>
#include <algorithm>

// FFTW3 include
// Make sure to link against fftw3
#include <fftw3.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace module2 {
namespace analysis {

void FFTAnalyzer::computePSD(const core::SignalData& data, std::vector<float>& psd, std::vector<float>& freqBins) {
    if (data.samples.empty()) {
        psd.clear();
        freqBins.clear();
        return;
    }

    size_t N = data.samples.size();
    psd.resize(N);
    freqBins.resize(N);

    // Allocate FFTW arrays
    fftwf_complex* in = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * N);
    fftwf_complex* out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * N);

    // Apply Hann window and copy to 'in'
    for (size_t i = 0; i < N; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (N - 1)));
        in[i][0] = data.samples[i].real() * window;
        in[i][1] = data.samples[i].imag() * window;
    }

    // Create plan and execute
    fftwf_plan plan = fftwf_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwf_execute(plan);

    // Calculate PSD and shift so 0 Hz is in the middle
    size_t halfN = N / 2;
    for (size_t i = 0; i < N; ++i) {
        // FFT shift index
        size_t shiftedIdx = (i < halfN) ? (i + (N - halfN)) : (i - halfN);
        
        float re = out[shiftedIdx][0];
        float im = out[shiftedIdx][1];
        
        // Magnitude squared
        psd[i] = (re * re + im * im) / N; // normalized by N
        
        // Frequency bins
        float freqOffset = static_cast<float>(shiftedIdx) / N;
        if (freqOffset >= 0.5f) {
            freqOffset -= 1.0f;
        }
        // Actually for the shifted array, i represents from -Fs/2 to Fs/2
        float f = (static_cast<float>(i) / N - 0.5f) * data.sampleRate + data.centerFrequency;
        freqBins[i] = f;
    }

    // Cleanup
    fftwf_destroy_plan(plan);
    fftwf_free(in);
    fftwf_free(out);
}

} // namespace analysis
} // namespace module2
