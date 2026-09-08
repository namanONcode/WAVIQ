#include "module1/SignalSessionModel.h"

#include <algorithm>
#include <stdexcept>

#include "module1/IqLoader.h"
#include "module1/LoaderExceptions.h"
#include "module1/LoaderFactory.h"

namespace module1 {
void SignalSessionModel::load_file(const std::string& path) {
    auto loader = LoaderFactory::create_loader(path);
    signal_ = loader->load(path);
    has_signal_ = true;
    has_labels_ = false;
    hdf5_.reset();
}
void SignalSessionModel::load_iq_with_metadata(const std::string& path, const IqMetadataInput& metadata) {
    IqLoader loader;
    signal_ = loader.load_with_metadata(path, metadata);
    has_signal_ = true;
    has_labels_ = false;
    hdf5_.reset();
}
void SignalSessionModel::open_hdf5(const std::string& path) { hdf5_ = std::make_unique<HdfIqFrameDataset>(path); has_signal_ = false; has_labels_ = false; }
void SignalSessionModel::load_hdf5_frame(size_t frame_index, double rate, IqAxisOrder order) {
    if (!hdf5_) throw std::logic_error("No HDF5 dataset is open");
    if (rate <= 0.0) throw MissingMetadataError("HDF5 dataset has no sample rate; provide one explicitly", {"sample_rate"});
    signal_ = hdf5_->load_frame(frame_index, rate, order);
    labels_ = hdf5_->labels_for_frame(frame_index);
    has_signal_ = true;
    has_labels_ = true;
}
AnalysisRegion SignalSessionModel::default_analysis_region(size_t fft_size) const {
    if (!has_signal_) throw std::logic_error("No signal is loaded");
    return {0, std::min(signal_.sample_count(), fft_size)};
}
void SignalSessionModel::validate_region(AnalysisRegion region) const {
    if (!has_signal_ || region.sample_count == 0 || region.start_sample > signal_.sample_count() || region.sample_count > signal_.sample_count() - region.start_sample)
        throw std::invalid_argument("Analysis region is outside the loaded signal");
}
size_t SignalSessionModel::hdf5_frame_count() const { if (!hdf5_) throw std::logic_error("No HDF5 dataset is open"); return hdf5_->frame_count(); }
std::string SignalSessionModel::current_modulation_name() const {
    if (!hdf5_ || !has_labels_) return "Unknown";
    for (const auto& [name, id] : hdf5_->modulation_label_map())
        if (id == labels_.modulation_id) return name;
    return "Unknown";
}
} // namespace module1
