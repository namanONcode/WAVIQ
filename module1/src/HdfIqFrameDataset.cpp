#include "module1/HdfIqFrameDataset.h"

#include <hdf5.h>

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <vector>

#include "module1/Float16.h"
#include "module1/LoaderExceptions.h"

namespace module1 {

namespace {

// Reads a scalar string attribute, handling both fixed-length and
// variable-length HDF5 string encodings (h5py can produce either
// depending on version/how the attribute was written -- we don't assume).
std::string read_string_attr(hid_t obj_id, const char* name) {
    if (!H5Aexists(obj_id, name)) {
        throw UnsupportedFormatError(std::string("Missing required attribute: ") + name);
    }
    hid_t attr = H5Aopen(obj_id, name, H5P_DEFAULT);
    if (attr < 0) {
        throw UnsupportedFormatError(std::string("Could not open attribute: ") + name);
    }
    hid_t type = H5Aget_type(attr);
    std::string result;

    if (H5Tis_variable_str(type) > 0) {
        char* raw = nullptr;
        if (H5Aread(attr, type, &raw) < 0) {
            H5Tclose(type);
            H5Aclose(attr);
            throw UnsupportedFormatError(std::string("Could not read variable-length attribute: ") + name);
        }
        if (raw) result = raw;
        H5free_memory(raw);
    } else {
        const size_t size = H5Tget_size(type);
        std::vector<char> buf(size + 1, '\0');
        if (H5Aread(attr, type, buf.data()) < 0) {
            H5Tclose(type);
            H5Aclose(attr);
            throw UnsupportedFormatError(std::string("Could not read fixed-length attribute: ") + name);
        }
        result.assign(buf.data(), size);
        // Trim trailing NULs some writers pad fixed-length strings with.
        const auto nul_pos = result.find('\0');
        if (nul_pos != std::string::npos) result.resize(nul_pos);
    }

    H5Tclose(type);
    H5Aclose(attr);
    return result;
}

int64_t read_int64_attr(hid_t obj_id, const char* name) {
    if (!H5Aexists(obj_id, name)) {
        throw UnsupportedFormatError(std::string("Missing required attribute: ") + name);
    }
    hid_t attr = H5Aopen(obj_id, name, H5P_DEFAULT);
    if (attr < 0) {
        throw UnsupportedFormatError(std::string("Could not open attribute: ") + name);
    }
    int64_t value = 0;
    const herr_t status = H5Aread(attr, H5T_NATIVE_INT64, &value);
    H5Aclose(attr);
    if (status < 0) {
        throw UnsupportedFormatError(std::string("Could not read integer attribute: ") + name);
    }
    return value;
}

// Reads the full extent (dims) of a dataset given its name. Throws
// UnsupportedFormatError if the dataset is missing.
std::vector<hsize_t> dataset_dims(hid_t file_id, const char* name, hid_t& out_dset) {
    if (!H5Lexists(file_id, name, H5P_DEFAULT)) {
        throw UnsupportedFormatError(std::string("Missing required dataset: ") + name);
    }
    out_dset = H5Dopen2(file_id, name, H5P_DEFAULT);
    if (out_dset < 0) {
        throw UnsupportedFormatError(std::string("Could not open dataset: ") + name);
    }
    hid_t space = H5Dget_space(out_dset);
    const int rank = H5Sget_simple_extent_ndims(space);
    std::vector<hsize_t> dims(static_cast<size_t>(rank));
    H5Sget_simple_extent_dims(space, dims.data(), nullptr);
    H5Sclose(space);
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
            throw UnsupportedFormatError("mod2id_json attribute is not valid JSON: " + std::string(e.what()));
        }
        for (auto it = j.begin(); it != j.end(); ++it) {
            modulation_label_map_[it.key()] = it.value().get<int>();
        }

        // Validate /X against the file's own frame_len attribute and its
        // own shape, rather than trusting either alone.
        hid_t x_dset = -1;
        std::vector<hsize_t> x_dims = dataset_dims(file_id_, "X", x_dset);
        if (x_dims.size() != 3 || x_dims[2] != 2) {
            H5Dclose(x_dset);
            throw UnsupportedFormatError(
                "/X does not have the expected [frames, samples, 2] shape in: " + path);
        }
        if (static_cast<int64_t>(x_dims[1]) != frame_len_attr) {
            H5Dclose(x_dset);
            throw UnsupportedFormatError(
                "/X's per-frame sample count (" + std::to_string(x_dims[1]) +
                ") does not match the file's frame_len attribute (" +
                std::to_string(frame_len_attr) + ") in: " + path);
        }
        hid_t x_type = H5Dget_type(x_dset);
        const bool x_is_float16 = (H5Tget_class(x_type) == H5T_FLOAT) && (H5Tget_size(x_type) == 2);
        H5Tclose(x_type);
        H5Dclose(x_dset);
        if (!x_is_float16) {
            throw UnsupportedFormatError(
                "/X is not a 2-byte floating point dataset (expected float16) in: " + path);
        }

        frame_count_ = static_cast<size_t>(x_dims[0]);
        frame_length_ = static_cast<size_t>(x_dims[1]);

        // Cross-check the three label datasets exist and agree on frame count.
        for (const char* name : {"y_chan", "y_mod", "y_snr"}) {
            hid_t d = -1;
            std::vector<hsize_t> dims = dataset_dims(file_id_, name, d);
            H5Dclose(d);
            if (dims.size() != 1 || dims[0] != x_dims[0]) {
                throw UnsupportedFormatError(
                    std::string("/") + name + " shape does not match /X's frame count in: " + path);
            }
        }
    } catch (...) {
        close();
        throw;
    }
}

HdfIqFrameDataset::~HdfIqFrameDataset() { close(); }

void HdfIqFrameDataset::close() {
    if (file_id_ >= 0) {
        H5Fclose(file_id_);
        file_id_ = -1;
    }
}

FrameLabels HdfIqFrameDataset::labels_for_frame(size_t frame_index) const {
    if (frame_index >= frame_count_) {
        throw std::out_of_range("frame_index " + std::to_string(frame_index) +
                                 " out of range (frame_count = " + std::to_string(frame_count_) + ")");
    }

    FrameLabels labels;

    auto read_scalar = [&](const char* name, hid_t mem_type, void* out) {
        hid_t dset = H5Dopen2(file_id_, name, H5P_DEFAULT);
        hid_t space = H5Dget_space(dset);
        hsize_t start = frame_index;
        hsize_t count = 1;
        H5Sselect_hyperslab(space, H5S_SELECT_SET, &start, nullptr, &count, nullptr);
        hid_t mem_space = H5Screate_simple(1, &count, nullptr);
        H5Dread(dset, mem_type, mem_space, space, H5P_DEFAULT, out);
        H5Sclose(mem_space);
        H5Sclose(space);
        H5Dclose(dset);
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
        throw std::out_of_range("frame_index " + std::to_string(frame_index) +
                                 " out of range (frame_count = " + std::to_string(frame_count_) + ")");
    }
    if (sample_rate_hz <= 0.0) {
        throw MissingMetadataError(
            "This HDF5 dataset does not provide a sample rate anywhere (checked all file-level "
            "and dataset-level attributes) -- it must be supplied explicitly.",
            {"sample_rate"});
    }

    hid_t x_dset = H5Dopen2(file_id_, "X", H5P_DEFAULT);
    hid_t x_type = H5Dget_type(x_dset); // the file's actual 2-byte float type -- used as-is for the
                                         // memory type too, so HDF5 performs a raw byte copy with NO
                                         // type conversion (see Float16.h for why that matters).
    hid_t space = H5Dget_space(x_dset);

    hsize_t start[3] = {static_cast<hsize_t>(frame_index), 0, 0};
    hsize_t count[3] = {1, static_cast<hsize_t>(frame_length_), 2};
    H5Sselect_hyperslab(space, H5S_SELECT_SET, start, nullptr, count, nullptr);
    hid_t mem_space = H5Screate_simple(3, count, nullptr);

    std::vector<uint16_t> raw(frame_length_ * 2);
    const herr_t status = H5Dread(x_dset, x_type, mem_space, space, H5P_DEFAULT, raw.data());

    H5Sclose(mem_space);
    H5Sclose(space);
    H5Tclose(x_type);
    H5Dclose(x_dset);

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
    metadata.iq_arrangement = IqArrangement::InterleavedIQ; // closest existing fit; not literally
                                                              // interleaved bytes on disk, but the
                                                              // same "paired I/Q per sample" shape
    metadata.byte_order = ByteOrder::Little; // not meaningful for this source -- HDF5 abstracts
                                              // on-disk byte order internally; kept at the default
                                              // since this field doesn't apply here
    metadata.source_path = path_ + "#frame" + std::to_string(frame_index);
    metadata.metadata_source = "hdf5_manual_sample_rate"; // sample rate was supplied by the caller,
                                                            // never read from the file

    return ComplexSignal(std::move(samples), std::move(metadata));
}

} // namespace module1
