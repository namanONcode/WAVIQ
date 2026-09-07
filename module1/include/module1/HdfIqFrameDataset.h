#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "module1/HdfFrameLabels.h"
#include "module1/SignalTypes.h"

namespace module1 {

// How the trailing axis of /X (shape [frame, sample, 2]) maps to I and Q.
//
// IMPORTANT: this is NOT verifiable from the file itself. Neither the
// file-level attrs nor the dataset attrs name the two channels -- there is
// no "0=I,1=Q" attribute anywhere in the schema we inspected. IThenQ is
// used as the default here purely as the more common convention, NOT
// because the file confirms it. Treat any constellation/demod result as
// suspect until this is confirmed against the dataset's paper or authors,
// and prefer swapping this enum over silently trusting the default.
enum class IqAxisOrder {
    IThenQ, // axis index 0 = I, axis index 1 = Q (default, UNVERIFIED)
    QThenI, // axis index 0 = Q, axis index 1 = I
};

// Reads the Mendeley "dataset3_iq_frames.h5"-schema HDF5 files (as inspected
// from subset_train/val/test.h5): file-level attrs channel_meaning,
// frame_len, mod2id_json, source, subset_name; datasets /X (float16,
// [N,1024,2]), /y_chan (int8, [N]), /y_mod (int16, [N]), /y_snr (int16, [N]).
//
// This is a standalone adapter, NOT an ISignalLoader. ISignalLoader::load()
// maps one file to one ComplexSignal, which doesn't fit a file containing
// tens of thousands of independent labeled frames -- callers ask for a
// specific frame by index instead. WavLoader/IqLoader/LoaderFactory are
// untouched by this class's existence.
//
// The file provides no sample rate anywhere (checked: absent from every
// file-level attr and every dataset-level attr). Per the existing
// "never guess metadata" rule, load_frame() requires the caller to supply
// one explicitly and throws MissingMetadataError if it isn't positive --
// this reuses the exact exception IqLoader already uses for the same
// situation on raw .iq files.
class HdfIqFrameDataset {
public:
    explicit HdfIqFrameDataset(const std::string& path);
    ~HdfIqFrameDataset();

    // Copying disabled (owns an open HDF5 file descriptor).
    HdfIqFrameDataset(const HdfIqFrameDataset&) = delete;
    HdfIqFrameDataset& operator=(const HdfIqFrameDataset&) = delete;

    // Movable following RAII Rule of Five.
    HdfIqFrameDataset(HdfIqFrameDataset&& other) noexcept;
    HdfIqFrameDataset& operator=(HdfIqFrameDataset&& other) noexcept;

    size_t frame_count() const { return frame_count_; }
    size_t frame_length() const { return frame_length_; }
    const std::string& channel_meaning() const { return channel_meaning_; }
    const std::string& source_attr() const { return source_attr_; }
    const std::string& subset_name() const { return subset_name_; }
    const ModulationLabelMap& modulation_label_map() const { return modulation_label_map_; }

    // Throws FrameIndexOutOfRangeError if frame_index >= frame_count().
    FrameLabels labels_for_frame(size_t frame_index) const;

    // Throws FrameIndexOutOfRangeError if frame_index >= frame_count().
    // Throws MissingMetadataError if sample_rate_hz <= 0 -- see class
    // comment above; this file format never supplies one.
    ComplexSignal load_frame(size_t frame_index, double sample_rate_hz,
                              IqAxisOrder order = IqAxisOrder::IThenQ) const;

private:
    std::string path_;
    long long file_id_ = -1; // hid_t, kept as long long to avoid leaking <hdf5.h> into this header

    size_t frame_count_ = 0;
    size_t frame_length_ = 0;
    std::string channel_meaning_;
    std::string source_attr_;
    std::string subset_name_;
    ModulationLabelMap modulation_label_map_;

    void close();
};

} // namespace module1
