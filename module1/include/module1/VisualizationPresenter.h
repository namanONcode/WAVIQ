#pragma once
// ============================================================================
// VisualizationPresenter — MVP Presenter (coordinates View ↔ Model/DSP)
// ============================================================================
//
// The Presenter is the sole coordinator between:
//   - The VIEW (IVisualizationView): a passive display-only interface.
//   - The MODEL: ComplexSignal objects produced by Module 1's ingestion
//     layer (WavLoader, IqLoader, HdfIqFrameDataset).
//
// Key architectural constraints (review requirements):
//   - NO API layer.  NO Controller.  NO REST/network/backend.
//   - This is a STANDALONE DESKTOP APPLICATION.
//   - DSP/processing (FFT, feature extraction) is kept SEPARATE from the
//     GUI — the Presenter invokes DSP operations and passes results to
//     the View.  The View never performs DSP itself.
//   - The existing Module 1 → Module 2 → Module 3 → Module 1 flow is
//     preserved.  This Presenter sits in Module 1's output/view layer.
//
// Typical usage:
//   1. Application creates a concrete View (e.g. QtVisualizationView).
//   2. Application creates VisualizationPresenter(&view).
//   3. Application loads a ComplexSignal via loaders / HdfIqFrameDataset.
//   4. Application calls presenter.on_signal_loaded(signal).
//   5. Presenter pushes pre-processed data to the View's render methods.
//
// For HDF5 frame-based datasets:
//   6. Application calls presenter.on_frame_loaded(signal, labels).
//   7. Presenter pushes both the signal and label information to the View.
// ============================================================================

#include <string>

#include "module1/HdfFrameLabels.h"
#include "module1/SignalTypes.h"
#include "module1/VisualizationView.h"

namespace module1 {

class VisualizationPresenter {
public:
    // The Presenter does NOT own the View; lifetime is managed externally.
    explicit VisualizationPresenter(IVisualizationView* view)
        : view_(view) {
        if (view_) {
            view_->set_presenter(this);
        }
    }

    // --- Event handlers (called by the application layer) ---

    // A new ComplexSignal was loaded (WAV, raw IQ, or HDF5 single frame).
    // The Presenter coordinates pushing the data to the View.
    void on_signal_loaded(const ComplexSignal& signal) {
        current_signal_ = signal;
        update_view_with_signal();
    }

    // An HDF5 frame was loaded along with its labels.
    // This is the entry point for the frame-browsing workflow.
    void on_frame_loaded(const ComplexSignal& signal,
                         const FrameLabels& labels,
                         const std::string& modulation_name = "") {
        current_signal_ = signal;
        current_labels_ = labels;
        has_labels_ = true;

        update_view_with_signal();

        if (view_) {
            view_->display_frame_info(
                modulation_name,
                labels.channel_condition,
                labels.snr_db);
        }
    }

    // --- Module 3 Integration Handlers ---

    // A complete demodulated and error-corrected payload is ready
    void on_fec_decoded(const std::vector<uint8_t>& payload, float bit_error_rate, bool decode_success) {
        if (view_) {
            view_->render_decoded_bitstream(payload);
            view_->display_fec_metrics(bit_error_rate, decode_success);
        }
    }

    // A sliding-window correlation was computed by Module 3
    void on_correlation_completed(const std::vector<float>& correlation_metrics) {
        if (view_) {
            view_->render_header_correlation(correlation_metrics);
        }
    }

    // --- Accessors (for View→Presenter queries, if needed) ---
    const ComplexSignal& current_signal() const { return current_signal_; }
    bool has_labels() const { return has_labels_; }
    const FrameLabels& current_labels() const { return current_labels_; }

private:
    void update_view_with_signal() {
        if (!view_) return;

        view_->display_metadata(current_signal_.metadata());
        view_->render_time_domain(current_signal_.samples());
        view_->render_constellation(current_signal_.samples());

        // FFT and Waterfall are DSP operations — they would be computed HERE (in the
        // Presenter or by a DSP utility function), NOT in the View.
        // When an FFT/Spectrogram module is implemented:
        //   auto spectrum = dsp::compute_magnitude_spectrum(current_signal_.samples());
        //   view_->render_fft(spectrum);
        //   auto spectrogram = dsp::compute_spectrogram(current_signal_.samples());
        //   view_->render_waterfall(spectrogram);
    }

    IVisualizationView* view_;
    ComplexSignal current_signal_;
    FrameLabels current_labels_{};
    bool has_labels_ = false;
};

} // namespace module1
