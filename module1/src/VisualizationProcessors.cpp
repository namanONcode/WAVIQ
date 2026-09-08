#include "module1/VisualizationProcessors.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
#include <stdexcept>

namespace module1 {
namespace {

constexpr float kPi = 3.14159265358979323846f;

bool is_power_of_two(size_t value) {
    return value >= 64 && value <= 65536 && (value & (value - 1)) == 0;
}

void validate_region(const ComplexSignal& signal, AnalysisRegion region) {
    if (region.sample_count == 0 || region.start_sample > signal.sample_count() ||
        region.sample_count > signal.sample_count() - region.start_sample) {
        throw std::invalid_argument("AnalysisRegion must be non-empty and lie within ComplexSignal samples");
    }
}

void validate_config(const VisualizationAnalysisConfig& config) {
    if (!is_power_of_two(config.fft_size) || !is_power_of_two(config.stft_fft_size) ||
        !is_power_of_two(config.stft_window_size) || config.stft_fft_size != config.stft_window_size ||
        config.stft_hop_size == 0 || config.stft_hop_size > config.stft_window_size ||
        config.waveform_point_budget == 0 || config.constellation_point_budget == 0 || config.db_floor >= 0.0f) {
        throw std::invalid_argument("Invalid visualization analysis configuration");
    }
}

std::vector<float> hann_window(size_t count) {
    std::vector<float> window(count);
    if (count == 1) { window[0] = 1.0f; return window; }
    for (size_t i = 0; i < count; ++i) {
        window[i] = 0.5f - 0.5f * std::cos(2.0f * kPi * static_cast<float>(i) / static_cast<float>(count - 1));
    }
    return window;
}

void fft_in_place(std::vector<std::complex<float>>& values) {
    const size_t n = values.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(values[i], values[j]);
    }
    for (size_t length = 2; length <= n; length <<= 1) {
        const float angle = -2.0f * kPi / static_cast<float>(length);
        const std::complex<float> w_length(std::cos(angle), std::sin(angle));
        for (size_t base = 0; base < n; base += length) {
            std::complex<float> w(1.0f, 0.0f);
            const size_t half = length / 2;
            for (size_t j = 0; j < half; ++j) {
                const std::complex<float> u = values[base + j];
                const std::complex<float> v = values[base + j + half] * w;
                values[base + j] = u + v;
                values[base + j + half] = u - v;
                w *= w_length;
            }
        }
    }
}

FrequencyAxisUnit frequency_unit(const ComplexSignal& signal) {
    return signal.sample_rate() > 0.0 ? FrequencyAxisUnit::Hertz : FrequencyAxisUnit::CyclesPerSample;
}

std::vector<double> centered_frequencies(size_t n, double sample_rate) {
    std::vector<double> result(n);
    const bool have_rate = sample_rate > 0.0;
    for (size_t k = 0; k < n; ++k) {
        const double normalized = -0.5 + static_cast<double>(k) / static_cast<double>(n);
        result[k] = have_rate ? normalized * sample_rate : normalized;
    }
    return result;
}

float db_from_power(float power, float floor) {
    if (!(power > 0.0f) || !std::isfinite(power)) return floor;
    return std::max(floor, 10.0f * std::log10(power));
}

std::vector<std::complex<float>> transformed_region(const ComplexSignal& signal, AnalysisRegion region,
                                                    size_t fft_size, const std::vector<float>& window) {
    validate_region(signal, region);
    if (region.sample_count > fft_size) {
        throw std::invalid_argument("AnalysisRegion exceeds requested FFT size; choose a smaller region or larger FFT");
    }
    std::vector<std::complex<float>> values(fft_size, {0.0f, 0.0f});
    for (size_t i = 0; i < region.sample_count; ++i) values[i] = signal.samples()[region.start_sample + i] * window[i];
    fft_in_place(values);
    return values;
}

struct FrequencyDomainResult {
    std::vector<double> frequency;
    std::vector<float> magnitude_db;
    std::vector<float> power_db;
    FrequencyAxisUnit unit;
};

FrequencyDomainResult make_frequency_domain(const ComplexSignal& signal, AnalysisRegion region,
                                            const VisualizationAnalysisConfig& config) {
    validate_config(config);
    std::vector<float> window = hann_window(config.fft_size);
    float window_sum = 0.0f;
    for (float value : window) window_sum += value;
    std::vector<std::complex<float>> fft = transformed_region(signal, region, config.fft_size, window);
    FrequencyDomainResult result{centered_frequencies(config.fft_size, signal.sample_rate()), {}, {}, frequency_unit(signal)};
    result.magnitude_db.resize(config.fft_size);
    result.power_db.resize(config.fft_size);
    const size_t shift = config.fft_size / 2;
    for (size_t k = 0; k < config.fft_size; ++k) {
        const float magnitude = std::abs(fft[(k + shift) % config.fft_size]) / window_sum;
        const float power = magnitude * magnitude;
        result.magnitude_db[k] = db_from_power(magnitude * magnitude, config.db_floor);
        result.power_db[k] = db_from_power(power, config.db_floor);
    }
    return result;
}

} // namespace

WaveformData WaveformProcessor::make_data(const ComplexSignal& signal, AnalysisRegion region, size_t point_budget) {
    validate_region(signal, region);
    if (point_budget == 0) throw std::invalid_argument("Waveform point budget must be positive");
    WaveformData output;
    output.x_unit = signal.sample_rate() > 0.0 ? TimeAxisUnit::Seconds : TimeAxisUnit::Samples;
    const auto& samples = signal.samples();
    const size_t end = region.start_sample + region.sample_count;
    if (point_budget == 1) {
        const double x = signal.sample_rate() > 0.0 ? static_cast<double>(region.start_sample) / signal.sample_rate() : static_cast<double>(region.start_sample);
        output.points.push_back({region.start_sample, x, samples[region.start_sample].real(), samples[region.start_sample].imag()});
        return output;
    }
    if (region.sample_count <= point_budget) {
        output.points.reserve(region.sample_count);
        for (size_t i = region.start_sample; i < end; ++i) {
            const double x = signal.sample_rate() > 0.0 ? static_cast<double>(i) / signal.sample_rate() : static_cast<double>(i);
            output.points.push_back({i, x, samples[i].real(), samples[i].imag()});
        }
        return output;
    }
    // Reserve endpoints. Four candidates per bucket retain I and Q extrema
    // independently without exceeding the caller's point budget.
    if (point_budget < 6) {
        const double start_x = signal.sample_rate() > 0.0 ? static_cast<double>(region.start_sample) / signal.sample_rate() : static_cast<double>(region.start_sample);
        const double end_x = signal.sample_rate() > 0.0 ? static_cast<double>(end - 1) / signal.sample_rate() : static_cast<double>(end - 1);
        output.points.push_back({region.start_sample, start_x, samples[region.start_sample].real(), samples[region.start_sample].imag()});
        output.points.push_back({end - 1, end_x, samples[end - 1].real(), samples[end - 1].imag()});
        return output;
    }
    const size_t bucket_count = std::max<size_t>(1, (point_budget - 2) / 4);
    output.points.reserve(bucket_count * 4 + 2);
    for (size_t bucket = 0; bucket < bucket_count; ++bucket) {
        const size_t begin = region.start_sample + bucket * region.sample_count / bucket_count;
        const size_t bucket_end = region.start_sample + (bucket + 1) * region.sample_count / bucket_count;
        size_t min_i = begin, max_i = begin, min_q = begin, max_q = begin;
        for (size_t i = begin + 1; i < bucket_end; ++i) {
            if (samples[i].real() < samples[min_i].real()) min_i = i;
            if (samples[i].real() > samples[max_i].real()) max_i = i;
            if (samples[i].imag() < samples[min_q].imag()) min_q = i;
            if (samples[i].imag() > samples[max_q].imag()) max_q = i;
        }
        const auto append = [&](size_t index) {
            const double x = signal.sample_rate() > 0.0 ? static_cast<double>(index) / signal.sample_rate() : static_cast<double>(index);
            output.points.push_back({index, x, samples[index].real(), samples[index].imag()});
        };
        std::vector<size_t> indices{min_i, max_i, min_q, max_q};
        std::sort(indices.begin(), indices.end());
        indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
        for (size_t index : indices) append(index);
    }
    if (output.points.front().source_sample != region.start_sample) output.points.insert(output.points.begin(), {region.start_sample, signal.sample_rate() > 0.0 ? static_cast<double>(region.start_sample) / signal.sample_rate() : static_cast<double>(region.start_sample), samples[region.start_sample].real(), samples[region.start_sample].imag()});
    if (output.points.back().source_sample != end - 1) output.points.push_back({end - 1, signal.sample_rate() > 0.0 ? static_cast<double>(end - 1) / signal.sample_rate() : static_cast<double>(end - 1), samples[end - 1].real(), samples[end - 1].imag()});
    return output;
}

ConstellationData ConstellationProcessor::make_data(const ComplexSignal& signal, AnalysisRegion region, size_t point_budget) {
    validate_region(signal, region);
    if (point_budget == 0) throw std::invalid_argument("Constellation point budget must be positive");
    ConstellationData output;
    const size_t count = std::min(region.sample_count, point_budget);
    output.points.reserve(count);
    const auto& samples = signal.samples();
    for (size_t i = 0; i < count; ++i) {
        const size_t offset = count == 1 ? 0 : i * (region.sample_count - 1) / (count - 1);
        const size_t index = region.start_sample + offset;
        output.points.push_back({samples[index].real(), samples[index].imag(), index});
    }
    return output;
}

SpectrumData SpectrumProcessor::make_magnitude_data(const ComplexSignal& signal, AnalysisRegion region, const VisualizationAnalysisConfig& config) {
    auto values = make_frequency_domain(signal, region, config);
    return {std::move(values.frequency), std::move(values.magnitude_db), values.unit};
}

PowerSpectrumData SpectrumProcessor::make_power_data(const ComplexSignal& signal, AnalysisRegion region, const VisualizationAnalysisConfig& config) {
    auto values = make_frequency_domain(signal, region, config);
    return {std::move(values.frequency), std::move(values.power_db), values.unit};
}

SpectrogramData SpectrogramProcessor::make_data(const ComplexSignal& signal, AnalysisRegion region, const VisualizationAnalysisConfig& config) {
    validate_config(config);
    validate_region(signal, region);
    SpectrogramData output;
    output.time_unit = signal.sample_rate() > 0.0 ? TimeAxisUnit::Seconds : TimeAxisUnit::Samples;
    output.frequency_unit = frequency_unit(signal);
    output.frequency = centered_frequencies(config.stft_fft_size, signal.sample_rate());
    if (region.sample_count < config.stft_window_size) return output;
    const auto window = hann_window(config.stft_window_size);
    float window_sum = 0.0f;
    for (float value : window) window_sum += value;
    const size_t frame_count = 1 + (region.sample_count - config.stft_window_size) / config.stft_hop_size;
    output.time.reserve(frame_count);
    output.power_db_relative.reserve(frame_count);
    for (size_t frame = 0; frame < frame_count; ++frame) {
        const size_t offset = frame * config.stft_hop_size;
        std::vector<std::complex<float>> values(config.stft_fft_size, {0.0f, 0.0f});
        for (size_t i = 0; i < config.stft_window_size; ++i) values[i] = signal.samples()[region.start_sample + offset + i] * window[i];
        fft_in_place(values);
        std::vector<float> row(config.stft_fft_size);
        for (size_t k = 0; k < config.stft_fft_size; ++k) {
            const float magnitude = std::abs(values[(k + config.stft_fft_size / 2) % config.stft_fft_size]) / window_sum;
            row[k] = db_from_power(magnitude * magnitude, config.db_floor);
        }
        const double center = static_cast<double>(region.start_sample + offset + config.stft_window_size / 2);
        output.time.push_back(signal.sample_rate() > 0.0 ? center / signal.sample_rate() : center);
        output.power_db_relative.push_back(std::move(row));
    }
    return output;
}

VisualizationProducts VisualizationProcessor::make_products(const ComplexSignal& signal, const VisualizationRequest& request,
                                                            const VisualizationAnalysisConfig& config) {
    VisualizationProducts output;
    if (request.waveform) { output.waveform = WaveformProcessor::make_data(signal, request.region, config.waveform_point_budget); output.has_waveform = true; }
    if (request.constellation) { output.constellation = ConstellationProcessor::make_data(signal, request.region, config.constellation_point_budget); output.has_constellation = true; }
    if (request.spectrum) { output.spectrum = SpectrumProcessor::make_magnitude_data(signal, request.region, config); output.has_spectrum = true; }
    if (request.power_spectrum) { output.power_spectrum = SpectrumProcessor::make_power_data(signal, request.region, config); output.has_power_spectrum = true; }
    if (request.spectrogram) { output.spectrogram = SpectrogramProcessor::make_data(signal, request.region, config); output.has_spectrogram = true; }
    return output;
}

} // namespace module1
