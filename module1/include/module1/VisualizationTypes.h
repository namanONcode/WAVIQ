#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace module1 {

// A half-open region [start_sample, start_sample + sample_count) in the
// original, full-resolution ComplexSignal.  Presentation code must never use
// this to replace or mutate that signal.
struct AnalysisRegion {
    size_t start_sample = 0;
    size_t sample_count = 0;
};

enum class TimeAxisUnit { Samples, Seconds };
enum class FrequencyAxisUnit { CyclesPerSample, Hertz };

struct WaveformPoint {
    size_t source_sample = 0;
    double x = 0.0;
    float i = 0.0f;
    float q = 0.0f;
};

struct WaveformData {
    std::vector<WaveformPoint> points;
    TimeAxisUnit x_unit = TimeAxisUnit::Samples;
};

struct ConstellationPoint {
    float i = 0.0f;
    float q = 0.0f;
    size_t source_sample = 0;
};

struct ConstellationData {
    std::vector<ConstellationPoint> points;
};

struct SpectrumData {
    std::vector<double> frequency;
    std::vector<float> magnitude_db_relative;
    FrequencyAxisUnit frequency_unit = FrequencyAxisUnit::CyclesPerSample;
};

struct PowerSpectrumData {
    std::vector<double> frequency;
    std::vector<float> power_db_relative;
    FrequencyAxisUnit frequency_unit = FrequencyAxisUnit::CyclesPerSample;
};

struct SpectrogramData {
    std::vector<double> time;
    std::vector<double> frequency;
    std::vector<std::vector<float>> power_db_relative;
    TimeAxisUnit time_unit = TimeAxisUnit::Samples;
    FrequencyAxisUnit frequency_unit = FrequencyAxisUnit::CyclesPerSample;
};

// Initial numerical contract. FFT sizes must be powers of two in [64, 65536].
// The default spectrum FFT is 1024. A selected region larger than fft_size is
// rejected rather than silently truncated; a shorter one is zero-padded.
struct VisualizationAnalysisConfig {
    size_t fft_size = 1024;
    size_t stft_window_size = 256;
    size_t stft_fft_size = 256;
    size_t stft_hop_size = 128;
    size_t waveform_point_budget = 4096;
    size_t constellation_point_budget = 10000;
    // Relative dB floor used for exact zero values and very small powers.
    float db_floor = -120.0f;
};

struct VisualizationRequest {
    AnalysisRegion region;
    bool waveform = false;
    bool constellation = false;
    bool spectrum = false;
    bool power_spectrum = false;
    bool spectrogram = false;
};

struct VisualizationProducts {
    bool has_waveform = false;
    WaveformData waveform;
    bool has_constellation = false;
    ConstellationData constellation;
    bool has_spectrum = false;
    SpectrumData spectrum;
    bool has_power_spectrum = false;
    PowerSpectrumData power_spectrum;
    bool has_spectrogram = false;
    SpectrogramData spectrogram;
};

} // namespace module1
