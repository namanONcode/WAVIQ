#pragma once
// ============================================================================
// IVisualizationView — MVP View interface (standalone desktop application)
// ============================================================================
//
// This is the View interface in the Model-View-Presenter (MVP) pattern used
// for WAVIQ's visualization/GUI architecture.
//
// Design decisions:
//   - NO API layer, NO Controller, NO REST/network/backend components.
//   - This is a STANDALONE DESKTOP APPLICATION — the View will be
//     implemented by a platform-specific GUI class (e.g. a Qt widget).
//   - DSP/processing is kept SEPARATE from the GUI: the View never
//     performs DSP.  It only receives pre-processed data from the Presenter.
//   - The Presenter coordinates between the View and the Model/DSP data.
//   - The existing Module 1 → Module 2 → Module 3 → Module 1 pipeline
//     flow is preserved: Module 1 ingests signals, Module 2 preprocesses,
//     Module 3 classifies, and Module 1's View layer displays results.
//
// Concrete View implementations (e.g. QtVisualizationView) will inherit
// from this interface.  For unit testing, a mock/stub view can be used
// (see test_module1.cpp: StubVisualizationView).
// ============================================================================

#include <complex>
#include <string>
#include <vector>

#include "module1/SignalTypes.h"

namespace module1 {

class VisualizationPresenter;

// Pure interface: the View only knows how to display data.  It never
// accesses loaders, HDF5 datasets, or DSP routines directly.
class IVisualizationView {
public:
    virtual ~IVisualizationView() = default;

    // Bind the view to its presenter (called once during construction).
    virtual void set_presenter(VisualizationPresenter* presenter) = 0;

    // --- Rendering callbacks (called by the Presenter) ---

    // Display time-domain I/Q waveform data.
    virtual void render_time_domain(
        const std::vector<std::complex<float>>& samples) = 0;

    // Display frequency-domain magnitude spectrum (pre-computed by DSP,
    // NOT by the View).
    virtual void render_fft(
        const std::vector<float>& magnitude_spectrum) = 0;

    // Display constellation diagram points.
    virtual void render_constellation(
        const std::vector<std::complex<float>>& samples) = 0;

    // Display waterfall (time-frequency domain) spectrogram.
    virtual void render_waterfall(
        const std::vector<std::vector<float>>& spectrogram) = 0;

    // Display signal metadata (sample rate, format, provenance, etc.).
    virtual void display_metadata(const SignalMetadata& metadata) = 0;

    // Display frame label information (modulation, channel, SNR).
    virtual void display_frame_info(
        const std::string& modulation_name,
        int channel_condition,
        int snr_db) = 0;

    // Standalone desktop application window lifecycle.
    virtual void show_window() = 0;

    // --- Module 3 Integration ---

    // Display the final error-corrected, extracted payload bits
    virtual void render_decoded_bitstream(const std::vector<uint8_t>& payload) = 0;

    // Display Forward Error Correction performance metrics
    virtual void display_fec_metrics(float bit_error_rate, bool decode_success) = 0;

    // Display the sliding window correlation metric for sync word matching
    virtual void render_header_correlation(const std::vector<float>& correlation_metric) = 0;
};

} // namespace module1
