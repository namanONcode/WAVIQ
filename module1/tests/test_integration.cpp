#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "module1/LoaderFactory.h"
#include "module1/VisualizationPresenter.h"
#include "module1/VisualizationView.h"
#include "module1/Logger.h"

using namespace module1;

class MockVisualizationView : public IVisualizationView {
public:
    void set_presenter(VisualizationPresenter*) override {}
    void render_time_domain(const std::vector<std::complex<float>>&) override {}
    void render_fft(const std::vector<float>&) override {}
    void render_constellation(const std::vector<std::complex<float>>&) override {}
    void render_waterfall(const std::vector<std::vector<float>>&) override {}
    void display_metadata(const SignalMetadata&) override {}
    void display_frame_info(const std::string&, int, int) override {}
    void show_window() override {}
    
    // Module 3 callbacks
    void render_decoded_bitstream(const std::vector<uint8_t>& payload) override {
        decoded_payload = payload;
    }
    void display_fec_metrics(float bit_error_rate, bool decode_success) override {
        ber = bit_error_rate;
        success = decode_success;
    }
    void render_header_correlation(const std::vector<float>& correlation_metric) override {
        correlation = correlation_metric;
    }
    void append_log(const std::string& message) override {
        logs.push_back(message);
        std::cout << "[GUI LOG] " << message << std::endl;
    }

    std::vector<uint8_t> decoded_payload;
    float ber = 1.0f;
    bool success = false;
    std::vector<float> correlation;
    std::vector<std::string> logs;
};

int main() {
    MockVisualizationView view;
    VisualizationPresenter presenter(&view);
    
    Logger::getInstance().setLogCallback([&view](const std::string& msg) {
        view.append_log(msg);
    });

    std::string testFile = "../testdata/real_subset.wav";
    
    std::cout << "Starting Integration Test..." << std::endl;
    presenter.load_signal_file(testFile);
    
    assert(view.success == true);
    assert(view.decoded_payload.size() > 0);
    assert(view.logs.size() > 0);
    
    // Verify file saving
    std::ifstream infile("output_payload.bin", std::ios::binary | std::ios::ate);
    assert(infile.is_open());
    std::streamsize size = infile.tellg();
    assert(size == view.decoded_payload.size());
    infile.close();
    
    std::cout << "Integration Test Passed! Extracted " << view.decoded_payload.size() << " payload bytes." << std::endl;
    return 0;
}
