#include "module1/HdfIqFrameDataset.h"

#include <hdf5.h>

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>
#include <vector>

#include "module1/Float16.h"
#include "module1/LoaderExceptions.h"

namespace module1 {

namespace {

// RAII wrapper for HDF5 hid_t identifiers to guarantee deterministic,
// exception-safe handle closure without resource leaks.
class HdfHandle {
public:
    using Closer = herr_t (*)(hid_t);

    HdfHandle() : id_(-1), closer_(nullptr) {}
    HdfHandle(hid_t id, Closer closer) : id_(id), closer_(closer) {}
    ~HdfHandle() { reset(); }

    HdfHandle(const HdfHandle&) = delete;
    HdfHandle& operator=(const HdfHandle&) = delete;

    HdfHandle(HdfHandle&& other) noexcept : id_(other.id_), closer_(other.closer_) {
        other.id_ = -1;
        other.closer_ = nullptr;
    }

    HdfHandle& operator=(HdfHandle&& other) noexcept {
        if (this != &other) {
            reset();
            id_ = other.id_;
            closer_ = other.closer_;
            other.id_ = -1;
            other.closer_ = nullptr;
        }
        return *this;
    }

    hid_t get() const { return id_; }
    bool is_valid() const { return id_ >= 0; }
    explicit operator bool() const { return is_valid(); }

    void reset(hid_t new_id = -1, Closer new_closer = nullptr) {
        if (id_ >= 0 && closer_) {
            closer_(id_);
        }
        id_ = new_id;
        closer_ = new_closer;
    }

private:
    hid_t id_ = -1;
    Closer closer_ = nullptr;
};

HdfHandle make_dataset_handle(hid_t id) { return HdfHandle(id, H5Dclose); }
HdfHandle make_space_handle(hid_t id)   { return HdfHandle(id, H5Sclose); }
HdfHandle make_type_handle(hid_t id)    { return HdfHandle(id, H5Tclose); }
HdfHandle make_attr_handle(hid_t id)    { return HdfHandle(id, H5Aclose); }

// Reads a scalar string attribute, handling both fixed-length and
// variable-length HDF5 string encodings.
std::string read_string_attr(hid_t obj_id, const char* name) {
    if (!H5Aexists(obj_id, name)) {
        throw Hdf5MalformedDatasetError(std::string("Missing required attribute: ") + name);
    }
    HdfHandle attr = make_attr_handle(H5Aopen(obj_id, name, H5P_DEFAULT));
    if (!attr) {
        throw Hdf5MalformedDatasetError(std::string("Could not open attribute: ") + name);
    }
    HdfHandle type = make_type_handle(H5Aget_type(attr.get()));
    std::string result;

    if (H5Tis_variable_str(type.get()) > 0) {
        char* raw = nullptr;
        if (H5Aread(attr.get(), type.get(), &raw) < 0) {
            throw Hdf5MalformedDatasetError(std::string("Could not read variable-length attribute: ") + name);
        }
        if (raw) result = raw;
        H5free_memory(raw);
    } else {
        const size_t size = H5Tget_size(type.get());
        std::vector<char> buf(size + 1, '\0');
        if (H5Aread(attr.get(), type.get(), buf.data()) < 0) {
            throw Hdf5MalformedDatasetError(std::string("Could not read fixed-length attribute: ") + name);
        }
        result.assign(buf.data(), size);
        const auto nul_pos = result.find('\0');
        if (nul_pos != std::string::npos) result.resize(nul_pos);
    }

    return result;
}

int64_t read_int64_attr(hid_t obj_id, const char* name) {
    if (!H5Aexists(obj_id, name)) {
        throw Hdf5MalformedDatasetError(std::string("Missing required attribute: ") + name);
    }
    HdfHandle attr = make_attr_handle(H5Aopen(obj_id, name, H5P_DEFAULT));
    if (!attr) {
        throw Hdf5MalformedDatasetError(std::string("Could not open attribute: ") + name);
    }
    int64_t value = 0;
    const herr_t status = H5Aread(attr.get(), H5T_NATIVE_INT64, &value);
    if (status < 0) {
        throw Hdf5MalformedDatasetError(std::string("Could not read integer attribute: ") + name);
    }
    return value;
}

// Reads the dimensions of a dataset given its name.
std::vector<hsize_t> dataset_dims(hid_t file_id, const char* name) {
    if (!H5Lexists(file_id, name, H5P_DEFAULT)) {
        throw Hdf5MalformedDatasetError(std::string("Missing required dataset: ") + name);
    }
    HdfHandle dset = make_dataset_handle(H5Dopen2(file_id, name, H5P_DEFAULT));
    if (!dset) {
        throw Hdf5MalformedDatasetError(std::string("Could not open dataset: ") + name);
    }
    HdfHandle space = make_space_handle(H5Dget_space(dset.get()));
    const int rank = H5Sget_simple_extent_ndims(space.get());
    std::vector<hsize_t> dims(static_cast<size_t>(rank));
    H5Sget_simple_extent_dims(space.get(), dims.data(), nullptr);
    return dims;
}

} // namespace

HdfIqFrameDataset::HdfIqFrameDataset(const std::string& path) : path_(path) {
    file_id_ = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file_id_ < 0) {
        throw FileIOError("Could not open HDF5 file: " + path);
    }

    try {
        channel_meaning_ = read_string_attr(file_id_, "channel_meaning");
        source_attr_ = read_string_attr(file_id_, "source");
        subset_name_ = read_string_attr(file_id_, "subset_name");
        const int64_t frame_len_attr = read_int64_attr(file_id_, "frame_len");
        const std::string mod2id_json = read_string_attr(file_id_, "mod2id_json");

        nlohmann::json j;
        try {
            j = nlohmann::json::parse(mod2id_json);
        } catch (const nlohmann::json::parse_error& e) {
            throw Hdf5MalformedDatasetError("mod2id_json attribute is not valid JSON: " + std::string(e.what()));
        }
        for (auto it = j.begin(); it != j.end(); ++it) {
            modulation_label_map_[it.key()] = it.value().get<int>();
        }

        // Validate /X against the file's own frame_len attribute and its own shape
        std::vector<hsize_t> x_dims;
        {
            if (!H5Lexists(file_id_, "X", H5P_DEFAULT)) {
                throw Hdf5MalformedDatasetError(std::string("Missing required dataset: X"));
            }
            HdfHandle x_dset = make_dataset_handle(H5Dopen2(file_id_, "X", H5P_DEFAULT));
            if (!x_dset) {
                throw Hdf5MalformedDatasetError(std::string("Could not open dataset: X"));
            }
            HdfHandle space = make_space_handle(H5Dget_space(x_dset.get()));
            const int rank = H5Sget_simple_extent_ndims(space.get());
            x_dims.resize(static_cast<size_t>(rank));
            H5Sget_simple_extent_dims(space.get(), x_dims.data(), nullptr);

            if (x_dims.size() != 3 || x_dims[2] != 2) {
                throw Hdf5MalformedDatasetError(
                    "/X does not have the expected [frames, samples, 2] shape in: " + path);
            }
            if (static_cast<int64_t>(x_dims[1]) != frame_len_attr) {
                throw Hdf5MalformedDatasetError(
                    "/X's per-frame sample count (" + std::to_string(x_dims[1]) +
                    ") does not match the file's frame_len attribute (" +
                    std::to_string(frame_len_attr) + ") in: " + path);
            }
            HdfHandle x_type = make_type_handle(H5Dget_type(x_dset.get()));
            const bool x_is_float16 = (H5Tget_class(x_type.get()) == H5T_FLOAT) && (H5Tget_size(x_type.get()) == 2);
            if (!x_is_float16) {
                throw Hdf5MalformedDatasetError(
                    "/X is not a 2-byte floating point dataset (expected float16) in: " + path);
            }
        }

        frame_count_ = static_cast<size_t>(x_dims[0]);
        frame_length_ = static_cast<size_t>(x_dims[1]);

        // Cross-check the three label datasets exist and agree on frame count.
        for (const char* name : {"y_chan", "y_mod", "y_snr"}) {
            std::vector<hsize_t> dims = dataset_dims(file_id_, name);
            if (dims.size() != 1 || dims[0] != x_dims[0]) {
                throw Hdf5MalformedDatasetError(
                    std::string("/") + name + " shape does not match /X's frame count in: " + path);
            }
        }
    } catch (...) {
        close();
        throw;
    }
}

HdfIqFrameDataset::~HdfIqFrameDataset() {
    close();
}

HdfIqFrameDataset::HdfIqFrameDataset(HdfIqFrameDataset&& other) noexcept
    : path_(std::move(other.path_)),
      file_id_(other.file_id_),
      frame_count_(other.frame_count_),
      frame_length_(other.frame_length_),
      channel_meaning_(std::move(other.channel_meaning_)),
      source_attr_(std::move(other.source_attr_)),
      subset_name_(std::move(other.subset_name_)),
      modulation_label_map_(std::move(other.modulation_label_map_)) {
    other.file_id_ = -1;
    other.frame_count_ = 0;
    other.frame_length_ = 0;
}

HdfIqFrameDataset& HdfIqFrameDataset::operator=(HdfIqFrameDataset&& other) noexcept {
    if (this != &other) {
        close();
        path_ = std::move(other.path_);
        file_id_ = other.file_id_;
        frame_count_ = other.frame_count_;
        frame_length_ = other.frame_length_;
        channel_meaning_ = std::move(other.channel_meaning_);
        source_attr_ = std::move(other.source_attr_);
        subset_name_ = std::move(other.subset_name_);
        modulation_label_map_ = std::move(other.modulation_label_map_);

        other.file_id_ = -1;
        other.frame_count_ = 0;
        other.frame_length_ = 0;
    }
    return *this;
}

void HdfIqFrameDataset::close() {
    if (file_id_ >= 0) {
        H5Fclose(file_id_);
        file_id_ = -1;
    }
}

FrameLabels HdfIqFrameDataset::labels_for_frame(size_t frame_index) const {
    if (frame_index >= frame_count_) {
        throw FrameIndexOutOfRangeError(frame_index, frame_count_);
    }

    FrameLabels labels;

    auto read_scalar = [&](const char* name, hid_t mem_type, void* out) {
        HdfHandle dset = make_dataset_handle(H5Dopen2(file_id_, name, H5P_DEFAULT));
        HdfHandle space = make_space_handle(H5Dget_space(dset.get()));
        hsize_t start = frame_index;
        hsize_t count = 1;
        H5Sselect_hyperslab(space.get(), H5S_SELECT_SET, &start, nullptr, &count, nullptr);
        HdfHandle mem_space = make_space_handle(H5Screate_simple(1, &count, nullptr));
        H5Dread(dset.get(), mem_type, mem_space.get(), space.get(), H5P_DEFAULT, out);
    };

    int16_t mod_val = 0;
    int8_t chan_val = 0;
    int16_t snr_val = 0;
    read_scalar("y_mod", H5T_NATIVE_INT16, &mod_val);
    read_scalar("y_chan", H5T_NATIVE_INT8, &chan_val);
    read_scalar("y_snr", H5T_NATIVE_INT16, &snr_val);

    labels.modulation_id = mod_val;
    labels.channel_condition = chan_val;
    labels.snr_db = snr_val;
    return labels;
}

ComplexSignal HdfIqFrameDataset::load_frame(size_t frame_index, double sample_rate_hz, IqAxisOrder order) const {
    if (frame_index >= frame_count_) {
        throw FrameIndexOutOfRangeError(frame_index, frame_count_);
    }
    if (sample_rate_hz <= 0.0) {
        throw MissingMetadataError(
            "This HDF5 dataset does not provide a sample rate anywhere (checked all file-level "
            "and dataset-level attributes) -- it must be supplied explicitly.",
            {"sample_rate"});
    }

    HdfHandle x_dset = make_dataset_handle(H5Dopen2(file_id_, "X", H5P_DEFAULT));
    HdfHandle x_type = make_type_handle(H5Dget_type(x_dset.get()));
    HdfHandle space = make_space_handle(H5Dget_space(x_dset.get()));

    hsize_t start[3] = {static_cast<hsize_t>(frame_index), 0, 0};
    hsize_t count[3] = {1, static_cast<hsize_t>(frame_length_), 2};
    H5Sselect_hyperslab(space.get(), H5S_SELECT_SET, start, nullptr, count, nullptr);
    HdfHandle mem_space = make_space_handle(H5Screate_simple(3, count, nullptr));

    std::vector<uint16_t> raw(frame_length_ * 2);
    const herr_t status = H5Dread(x_dset.get(), x_type.get(), mem_space.get(), space.get(), H5P_DEFAULT, raw.data());

    if (status < 0) {
        throw FileIOError("Failed to read frame " + std::to_string(frame_index) + " from: " + path_);
    }

    std::vector<std::complex<float>> samples;
    samples.reserve(frame_length_);
    for (size_t i = 0; i < frame_length_; ++i) {
        const float a = half_to_float(raw[i * 2 + 0]);
        const float b = half_to_float(raw[i * 2 + 1]);
        if (order == IqAxisOrder::IThenQ) {
            samples.emplace_back(a, b);
        } else {
            samples.emplace_back(b, a);
        }
    }

    SignalMetadata metadata;
    metadata.sample_rate_hz = sample_rate_hz;
    metadata.source_format = "hdf5_iq_frame_dataset";
    metadata.sample_datatype = "float16(source)->float32";
    metadata.channel_count = 2;
    metadata.is_complex = true;
    metadata.iq_arrangement = IqArrangement::InterleavedIQ;
    metadata.byte_order = ByteOrder::Little;
    metadata.source_path = path_ + "#frame" + std::to_string(frame_index);
    metadata.metadata_source = "hdf5_manual_sample_rate";

    return ComplexSignal(std::move(samples), std::move(metadata));
}

} // namespace module1
