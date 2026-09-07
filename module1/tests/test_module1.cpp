// Lightweight test harness (no gtest dependency) covering:
//   1-3, 5-6:  HDF5 dataset adapter (real preview values, shapes, malformed
//              input, out-of-range, and visualization-input well-formedness)
//   4:         metadata handling (used when present; refused when absent)
//   7-10:      regression checks that WAV/IQ behavior is unchanged
//
// Run via `ctest` from the build directory, or directly as ./module1_tests.

#include <cmath>
#include <complex>
#include <cstdio>
#include <string>

#include "module1/HdfIqFrameDataset.h"
#include "module1/IqLoader.h"
#include "module1/LoaderExceptions.h"
#include "module1/LoaderFactory.h"
#include "module1/WavLoader.h"

using namespace module1;

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::printf("  FAIL: %s\n", description.c_str());
    }
}

bool approx(float a, float b, float eps = 1e-3f) {
    return std::fabs(a - b) <= eps;
}

std::string testdata(const std::string& filename) {
    return std::string(TESTDATA_DIR) + "/" + filename;
}

// ---------------------------------------------------------------- WAV ----

void test_wav_mono() {
    std::printf("test_wav_mono\n");
    WavLoader loader;
    ComplexSignal sig = loader.load(testdata("mono_pcm16.wav"));
    check(sig.sample_count() == 1000, "mono WAV has 1000 samples");
    check(!sig.metadata().is_complex, "mono WAV is not marked complex");
    check(sig.metadata().channel_count == 1, "mono WAV reports 1 channel");
    check(sig.metadata().metadata_source == "wav_header", "mono WAV metadata_source is wav_header");
    bool all_imag_zero = true;
    for (const auto& s : sig.samples()) {
        if (s.imag() != 0.0f) { all_imag_zero = false; break; }
    }
    check(all_imag_zero, "mono WAV samples have zero imaginary part");
}

void test_wav_stereo_as_iq() {
    std::printf("test_wav_stereo_as_iq\n");
    WavLoader loader;
    ComplexSignal sig = loader.load(testdata("iq_as_stereo.wav"));
    check(sig.sample_count() == 1000, "stereo WAV has 1000 frames");
    check(sig.metadata().is_complex, "stereo WAV is marked complex");
    check(sig.metadata().channel_count == 2, "stereo WAV reports 2 channels");
}

// -------------------------------------------------------------- raw IQ ----

void test_raw_iq_with_sidecar() {
    std::printf("test_raw_iq_with_sidecar\n");
    IqLoader loader;
    ComplexSignal sig = loader.load(testdata("capture_int16.iq"));
    check(sig.sample_count() == 500, "sidecar IQ has 500 samples");
    check(approx(static_cast<float>(sig.sample_rate()), 1000000.0f), "sidecar IQ sample rate is 1e6");
    check(sig.metadata().metadata_source == "sigmf_sidecar", "sidecar IQ metadata_source is sigmf_sidecar");
}

void test_raw_iq_without_metadata_refuses() {
    std::printf("test_raw_iq_without_metadata_refuses\n");
    IqLoader loader;
    bool threw_missing = false;
    try {
        loader.load(testdata("capture_float32_nosidecar.iq"));
    } catch (const MissingMetadataError& e) {
        threw_missing = true;
        bool has_sample_rate = false;
        for (const auto& f : e.missing_fields()) if (f == "sample_rate") has_sample_rate = true;
        check(has_sample_rate, "missing-metadata error lists sample_rate");
    }
    check(threw_missing, "IQ with no sidecar and no manual metadata throws MissingMetadataError");
}

void test_raw_iq_manual_metadata_works() {
    std::printf("test_raw_iq_manual_metadata_works\n");
    IqLoader loader;
    IqMetadataInput meta;
    meta.sample_rate_hz = 500000.0;
    meta.sample_datatype = "float32";
    meta.byte_order = "little";
    meta.source = "manual_entry";
    ComplexSignal sig = loader.load_with_metadata(testdata("capture_float32_nosidecar.iq"), meta);
    check(sig.sample_count() == 200, "manually-supplied-metadata IQ has 200 samples");
    check(sig.metadata().metadata_source == "manual_entry", "manual IQ metadata_source is manual_entry");
}

void test_unsupported_extension_rejected() {
    std::printf("test_unsupported_extension_rejected\n");
    bool threw = false;
    try {
        auto loader = make_loader_for(testdata("something.dat"));
        (void)loader;
    } catch (const UnsupportedFormatError&) {
        threw = true;
    }
    check(threw, "unsupported extension throws UnsupportedFormatError");
}

// ------------------------------------------------------------- HDF5 ----

void test_hdf5_structure_and_metadata() {
    std::printf("test_hdf5_structure_and_metadata\n");
    HdfIqFrameDataset ds(testdata("hdf5_fixture_from_real_preview.h5"));

    check(ds.frame_count() == 3, "fixture has 3 frames");
    check(ds.frame_length() == 1024, "fixture frame_length matches frame_len attr (1024)");
    check(ds.channel_meaning() == "0=clean, 1=multipath(ref)", "channel_meaning attr read correctly");

    const auto& map = ds.modulation_label_map();
    check(map.size() == 7, "modulation_label_map has 7 entries");
    check(map.count("BPSK") && map.at("BPSK") == 0, "BPSK maps to 0");
    check(map.count("WBFM") && map.at("WBFM") == 6, "WBFM maps to 6");
}

void test_hdf5_refuses_missing_sample_rate() {
    std::printf("test_hdf5_refuses_missing_sample_rate\n");
    HdfIqFrameDataset ds(testdata("hdf5_fixture_from_real_preview.h5"));
    bool threw_missing = false;
    try {
        ds.load_frame(0, /*sample_rate_hz=*/0.0);
    } catch (const MissingMetadataError& e) {
        threw_missing = true;
        bool has_sample_rate = false;
        for (const auto& f : e.missing_fields()) if (f == "sample_rate") has_sample_rate = true;
        check(has_sample_rate, "HDF5 missing-rate error lists sample_rate");
    }
    check(threw_missing, "HDF5 load_frame with no sample rate throws MissingMetadataError");
}

void test_hdf5_frame_values_match_real_preview() {
    std::printf("test_hdf5_frame_values_match_real_preview\n");
    HdfIqFrameDataset ds(testdata("hdf5_fixture_from_real_preview.h5"));

    // Arbitrary placeholder rate -- the dataset provides none (confirmed:
    // absent from every attribute we inspected), so this validates
    // conversion correctness only, not any real capture rate.
    const double placeholder_rate = 1.0;

    // Real preview values from your subset_test.h5 terminal output, frame 0.
    ComplexSignal f0 = ds.load_frame(0, placeholder_rate);
    check(f0.sample_count() == 1024, "frame 0 has 1024 samples");
    check(approx(f0.samples()[0].real(), -0.1172f) && approx(f0.samples()[0].imag(), 0.10156f),
          "frame 0 sample 0 matches real preview value");
    check(approx(f0.samples()[1].real(), -0.1172f) && approx(f0.samples()[1].imag(), 0.09375f),
          "frame 0 sample 1 matches real preview value");
    check(approx(f0.samples()[2].real(), -0.1172f) && approx(f0.samples()[2].imag(), 0.0703f),
          "frame 0 sample 2 matches real preview value");

    // Frame 1 and frame 2 real preview values.
    ComplexSignal f1 = ds.load_frame(1, placeholder_rate);
    check(approx(f1.samples()[0].real(), -0.1172f) && approx(f1.samples()[0].imag(), 0.1328f),
          "frame 1 sample 0 matches real preview value");

    ComplexSignal f2 = ds.load_frame(2, placeholder_rate);
    check(approx(f2.samples()[0].real(), 0.1875f) && approx(f2.samples()[0].imag(), 0.007812f),
          "frame 2 sample 0 matches real preview value");

    check(f0.metadata().metadata_source == "hdf5_manual_sample_rate",
          "HDF5 frame metadata_source records the rate as externally supplied");

    // QThenI should swap real/imag relative to the default IThenQ.
    ComplexSignal f0_swapped = ds.load_frame(0, placeholder_rate, IqAxisOrder::QThenI);
    check(approx(f0_swapped.samples()[0].real(), f0.samples()[0].imag()) &&
          approx(f0_swapped.samples()[0].imag(), f0.samples()[0].real()),
          "QThenI axis order swaps real/imag relative to IThenQ");
}

void test_hdf5_labels_match_real_preview() {
    std::printf("test_hdf5_labels_match_real_preview\n");
    HdfIqFrameDataset ds(testdata("hdf5_fixture_from_real_preview.h5"));
    FrameLabels l0 = ds.labels_for_frame(0);
    check(l0.modulation_id == 0, "frame 0 modulation_id matches real preview (0 = BPSK)");
    check(l0.channel_condition == 0, "frame 0 channel_condition matches real preview (0 = clean)");
    check(l0.snr_db == 20, "frame 0 snr_db matches real preview (20)");
}

void test_hdf5_visualization_input_is_well_formed() {
    std::printf("test_hdf5_visualization_input_is_well_formed\n");
    // This checks the ComplexSignal produced from a real HDF5 frame is
    // valid input for (future) visualization code -- non-empty, finite
    // values. It does NOT test any FFT/plotting code, since that part of
    // Module 1 hasn't been built yet.
    HdfIqFrameDataset ds(testdata("hdf5_fixture_from_real_preview.h5"));
    ComplexSignal sig = ds.load_frame(0, 1.0);
    check(sig.samples().size() == sig.sample_count(), "sample vector size matches sample_count()");
    bool all_finite = true;
    for (const auto& s : sig.samples()) {
        if (!std::isfinite(s.real()) || !std::isfinite(s.imag())) { all_finite = false; break; }
    }
    check(all_finite, "all decoded samples are finite (no NaN/Inf from float16 decode)");
}

void test_hdf5_malformed_frame_len_mismatch_rejected() {
    std::printf("test_hdf5_malformed_frame_len_mismatch_rejected\n");
    bool threw = false;
    try {
        HdfIqFrameDataset ds(testdata("hdf5_fixture_malformed_frame_len_mismatch.h5"));
        (void)ds;
    } catch (const UnsupportedFormatError&) {
        threw = true;
    }
    check(threw, "frame_len/shape mismatch throws UnsupportedFormatError");
}

void test_hdf5_malformed_missing_dataset_rejected() {
    std::printf("test_hdf5_malformed_missing_dataset_rejected\n");
    bool threw = false;
    try {
        HdfIqFrameDataset ds(testdata("hdf5_fixture_malformed_missing_dataset.h5"));
        (void)ds;
    } catch (const UnsupportedFormatError&) {
        threw = true;
    }
    check(threw, "missing required dataset (y_snr) throws UnsupportedFormatError");
}

void test_hdf5_out_of_range_frame_index() {
    std::printf("test_hdf5_out_of_range_frame_index\n");
    HdfIqFrameDataset ds(testdata("hdf5_fixture_from_real_preview.h5"));
    bool threw = false;
    try {
        ds.load_frame(999, 1.0);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    check(threw, "out-of-range frame index throws std::out_of_range");
}

} // namespace

int main() {
    test_wav_mono();
    test_wav_stereo_as_iq();
    test_raw_iq_with_sidecar();
    test_raw_iq_without_metadata_refuses();
    test_raw_iq_manual_metadata_works();
    test_unsupported_extension_rejected();
    test_hdf5_structure_and_metadata();
    test_hdf5_refuses_missing_sample_rate();
    test_hdf5_frame_values_match_real_preview();
    test_hdf5_labels_match_real_preview();
    test_hdf5_visualization_input_is_well_formed();
    test_hdf5_malformed_frame_len_mismatch_rejected();
    test_hdf5_malformed_missing_dataset_rejected();
    test_hdf5_out_of_range_frame_index();

    std::printf("\n%d/%d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures == 0 ? 0 : 1;
}
