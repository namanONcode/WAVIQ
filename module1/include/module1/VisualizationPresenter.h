#pragma once

#include <string>
#include <memory>
#include <exception>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cctype>

#include "module1/HdfFrameLabels.h"
#include "module1/SignalTypes.h"
#include "module1/VisualizationView.h"
#include "module1/SignalSessionModel.h"
#include "module1/VisualizationAnalysisExecutor.h"
#include "module1/Logger.h"

// Module 2 and 3 integrations
#include "../../../module2/include/core/AnalysisOrchestrator.hpp"
#include "../../../module3/include/FecDecoder.hpp"
#include "../../../module3/include/Correlator.hpp"
#include "../../../module3/include/Deinterleaver.hpp"

namespace module1 {

class VisualizationPresenter {
public:
    explicit VisualizationPresenter(IVisualizationView* view,
                                    IVisualizationAnalysisExecutor* executor = nullptr)
        : view_(view), executor_(executor ? executor : &inline_executor_) {
        if (view_) {
            view_->set_presenter(this);
        }
    }

    void on_signal_loaded(const ComplexSignal& signal) {
        current_signal_ = signal;
        update_view_with_signal();
        run_full_pipeline();
    }

    void on_frame_loaded(const ComplexSignal& signal,
                         const FrameLabels& labels,
                         const std::string& modulation_name = "") {
        current_signal_ = signal;
        current_labels_ = labels;
        has_labels_ = true;

        update_view_with_signal();
        run_full_pipeline();

        if (view_) {
            view_->display_frame_info(
                modulation_name,
                labels.channel_condition,
                labels.snr_db);
        }
    }

    // A complete demodulated and error-corrected payload is ready
    void on_fec_decoded(const std::vector<uint8_t>& payload, float bit_error_rate, bool decode_success) {
        if (view_) {
            view_->render_decoded_bitstream(payload);
            view_->display_fec_metrics(bit_error_rate, decode_success);
        }
    }

    void on_correlation_completed(const std::vector<float>& correlation_metrics) {
        if (view_) {
            view_->render_header_correlation(correlation_metrics);
        }
    }

    const ComplexSignal& current_signal() const { return current_signal_; }
    bool has_labels() const { return has_labels_; }
    const FrameLabels& current_labels() const { return current_labels_; }
    SignalSessionModel& session() { return session_; }
    const SignalSessionModel& session() const { return session_; }

    void load_signal_file(const std::string& path) {
        Logger::log("Loading file: " + path);
        session_.load_file(path);
        on_signal_loaded(session_.signal());
    }
    void load_iq_signal_with_metadata(const std::string& path, const IqMetadataInput& metadata) {
        Logger::log("Loading raw IQ file with metadata: " + path);
        session_.load_iq_with_metadata(path, metadata);
        on_signal_loaded(session_.signal());
    }
    void open_hdf5_dataset(const std::string& path) { 
        Logger::log("Opening HDF5 dataset: " + path);
        session_.open_hdf5(path); 
    }
    size_t hdf5_frame_count() const { return session_.hdf5_frame_count(); }
    void load_hdf5_dataset_frame(size_t frame_index, double externally_supplied_sample_rate_hz,
                                 IqAxisOrder order = IqAxisOrder::IThenQ) {
        Logger::log("Loading HDF5 frame: " + std::to_string(frame_index));
        session_.load_hdf5_frame(frame_index, externally_supplied_sample_rate_hz, order);
        on_frame_loaded(session_.signal(), session_.current_frame_labels(), session_.current_modulation_name());
    }

    void request_visualizations(const VisualizationRequest& request,
                                const VisualizationAnalysisConfig& config = {}) {
        if (current_signal_.sample_count() == 0) return;
        const size_t request_id = ++latest_request_id_;
        if (view_) view_->display_analysis_status("Analyzing…", true);
        Logger::log("Requesting visualizations (FFT, Spectrogram, etc.)...");
        executor_->submit(current_signal_, request, config,
            [this, request_id](VisualizationProducts products, std::exception_ptr error) {
                if (request_id != latest_request_id_) return;
                if (error) {
                    Logger::log("Visualization error occurred.");
                    try { std::rethrow_exception(error); }
                    catch (const std::exception& e) { if (view_) view_->display_analysis_error(e.what()); }
                    catch (...) { if (view_) view_->display_analysis_error("Visualization analysis failed"); }
                } else {
                    Logger::log("Visualizations updated.");
                    deliver_visualization_products(products);
                }
                if (view_) view_->display_analysis_status(error ? "Analysis failed" : "Ready", false);
            });
    }

    void drain_visualization_completions() { executor_->drain_completions(); }

private:
    void run_full_pipeline() {
        Logger::log("Starting DSP Pipeline (Module 2 -> Module 3)...");
        try {
            // 1. Prepare data for Module 2
            module2::core::SignalData data;
            data.samples = current_signal_.samples();
            data.sampleRate = current_signal_.metadata().sample_rate_hz;
            
            Logger::log("Module 2: Orchestrator analyzing " + std::to_string(data.samples.size()) + " samples.");
            auto m2_result = module2::core::AnalysisOrchestrator::analyze(data);
            
            if (!m2_result.bitStream) {
                Logger::log("Module 2 returned no valid bitstream. Pipeline aborted.");
                return;
            }
            
            Logger::log("Module 3: Passing bits to Deinterleaver...");
            module3::BlockDeinterleaver deint(15, 17); // 15 * 17 = 255 bits
            auto deinterleaved_stream = deint.deinterleave(m2_result.bitStream);
            
            if (!deinterleaved_stream) {
                Logger::log("Module 3: Deinterleaver failed (returned null).");
                return;
            }
            Logger::log("Module 3: Deinterleaver completed.");

            Logger::log("Module 3: Running Viterbi + RS FEC Decoder...");
            auto viterbi = std::make_shared<module3::ViterbiDecoder>(3, std::vector<int>{7, 5});
            auto rs = std::make_shared<module3::RSDecoder>(8, 255, 223);
            module3::ConcatenatedDecoder fec(viterbi, rs);
            
            auto fec_res = fec.decode(deinterleaved_stream);
            
            if (!fec_res.success) {
                Logger::log("Module 3: FEC Decoder FAILED.");
                on_fec_decoded(std::vector<uint8_t>{}, fec_res.bitErrorRate, false);
                return;
            }

            Logger::log("Module 3: FEC Decoder SUCCESS! Extracted " + std::to_string(fec_res.decodedBits->bits.size()) + " bits.");
            
            Logger::log("Module 3: Passing bits to Correlator...");
            std::vector<uint8_t> syncWord = {1, 0, 1, 0, 1, 1, 0, 0}; // Example sync word
            module3::SyncWordCorrelator correlator(syncWord, 2);
            auto corr_res = correlator.correlateAndExtract(fec_res.decodedBits->bits);
            
            on_correlation_completed(corr_res.correlationMetrics);
            Logger::log("Module 3: Correlation completed. Sync Found: " + std::string(corr_res.syncFound ? "YES" : "NO"));

            // Deliver the final payload bits from correlation result, or just FEC if sync wasn't found (for visualization)
            std::vector<uint8_t> final_payload = corr_res.syncFound ? corr_res.extractedPayload : fec_res.decodedBits->bits;
            
            handle_final_payload(final_payload);
            
            on_fec_decoded(final_payload, fec_res.bitErrorRate, true);

        } catch (const std::exception& e) {
            Logger::log("Pipeline Error: " + std::string(e.what()));
        }
    }

    void handle_final_payload(const std::vector<uint8_t>& payload) {
        if (payload.empty()) return;

        // 1. Save to File
        std::string out_path = "output_payload.bin";
        std::ofstream outfile(out_path, std::ios::binary);
        if (outfile) {
            outfile.write(reinterpret_cast<const char*>(payload.data()), payload.size());
            Logger::log("✅ Payload successfully saved to disk: " + out_path + " (" + std::to_string(payload.size()) + " bytes)");
        } else {
            Logger::log("❌ Failed to open " + out_path + " for writing.");
        }

        // 2. Hex/ASCII Dump to Logs
        std::ostringstream hex_dump;
        hex_dump << "\n--- PAYLOAD HEX DUMP ---\n";
        for (size_t i = 0; i < payload.size(); i += 16) {
            // Hex section
            for (size_t j = 0; j < 16; ++j) {
                if (i + j < payload.size()) {
                    hex_dump << std::hex << std::uppercase << std::setw(2) << std::setfill('0') 
                             << static_cast<int>(payload[i+j]) << " ";
                } else {
                    hex_dump << "   ";
                }
            }
            hex_dump << " | ";
            // ASCII section
            for (size_t j = 0; j < 16; ++j) {
                if (i + j < payload.size()) {
                    char c = payload[i+j];
                    hex_dump << (std::isprint(static_cast<unsigned char>(c)) ? c : '.');
                }
            }
            hex_dump << "\n";
        }
        hex_dump << "------------------------\n";
        Logger::log(hex_dump.str());
    }

    void update_view_with_signal() {
        if (!view_) return;
        view_->display_metadata(current_signal_.metadata());
        view_->render_time_domain(current_signal_.samples());
        view_->render_constellation(current_signal_.samples());
    }

    void deliver_visualization_products(const VisualizationProducts& products) {
        if (!view_) return;
        if (products.has_waveform) view_->render_waveform_data(products.waveform);
        if (products.has_constellation) view_->render_constellation_data(products.constellation);
        if (products.has_spectrum) view_->render_spectrum_data(products.spectrum);
        if (products.has_power_spectrum) view_->render_power_spectrum_data(products.power_spectrum);
        if (products.has_spectrogram) view_->render_spectrogram_data(products.spectrogram);
    }

    IVisualizationView* view_;
    ComplexSignal current_signal_;
    FrameLabels current_labels_{};
    bool has_labels_ = false;
    SignalSessionModel session_;
    InlineVisualizationAnalysisExecutor inline_executor_;
    IVisualizationAnalysisExecutor* executor_ = nullptr;
    size_t latest_request_id_ = 0;
};

} // namespace module1
