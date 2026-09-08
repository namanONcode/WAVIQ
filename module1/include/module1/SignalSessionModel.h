#pragma once

#include <memory>
#include <string>

#include "module1/HdfIqFrameDataset.h"
#include "module1/IqMetadata.h"
#include "module1/SignalTypes.h"
#include "module1/VisualizationTypes.h"

namespace module1 {

// Model-side loading/session state. It deliberately has no Qt dependency.
class SignalSessionModel {
public:
    void load_file(const std::string& path);
    void load_iq_with_metadata(const std::string& path, const IqMetadataInput& metadata);
    void open_hdf5(const std::string& path);
    void load_hdf5_frame(size_t frame_index, double externally_supplied_sample_rate_hz,
                         IqAxisOrder order = IqAxisOrder::IThenQ);

    bool has_signal() const { return has_signal_; }
    const ComplexSignal& signal() const { return signal_; }
    AnalysisRegion default_analysis_region(size_t fft_size) const;
    void validate_region(AnalysisRegion region) const;
    bool has_hdf5_dataset() const { return static_cast<bool>(hdf5_); }
    size_t hdf5_frame_count() const;
    FrameLabels current_frame_labels() const { return labels_; }
    bool has_frame_labels() const { return has_labels_; }
    std::string current_modulation_name() const;

private:
    ComplexSignal signal_;
    bool has_signal_ = false;
    std::unique_ptr<HdfIqFrameDataset> hdf5_;
    FrameLabels labels_{};
    bool has_labels_ = false;
};

} // namespace module1
