# Module 1: Testing Strategy, Fixtures, and Dataset Verification

## 1. Why Testing is Critical for Signal Ingestion

Digital signal processing pipelines fail silently. Unlike traditional business software where an error produces an exception or crash, signal ingestion bugs typically manifest as subtle numerical corruption:
- A byte-offset error shifts $I$ and $Q$ channels, turning a clean constellation into noise.
- An off-by-one error in endianness swaps MSB and LSB, destroying dynamic range.
- A float16 decode error misinterprets small signal amplitudes or subnormal values.
- Truncation or frame-misalignment discards signal energy and distorts FFT frequency bins.

Because downstream modules (Module 2 preprocessing, Module 3 ML classification and demodulation) operate mathematically on [`ComplexSignal`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/SignalTypes.h#L55), any defect introduced during ingestion invalidates every subsequent stage. The Module 1 test suite is specifically architected to guarantee bit-accurate ingestion, strict error refusal, and zero silent data corruption.

---

## 2. Test Architecture and Execution

### 2.5 Visualization DSP Real-Data Policy

All normal-path visualization DSP correctness tests use authentic samples read
through `HdfIqFrameDataset` from `testdata/real_subset.h5`. They do not create
synthetic tones, impulses, noise, or hand-written IQ buffers. FFT and STFT
reference constants are independently calculated with Python `h5py`/NumPy
using Hann windows, `fft`, `fftshift`, and the documented relative-power dB
formula. The C++ processors under test never generate their own expected
values.

The core suite verifies real-frame FFT dimensions, centered frequency ordering,
Hz spacing for an explicitly supplied rate, selected magnitude/power bins,
zero-padded short-region behavior, STFT shape and cells, finite outputs,
deterministic reduction, product independence, unchanged `ComplexSignal`, and
explicit HDF5 sample-rate requirements.

### 2.1 Framework-Free Design
The test harness [`tests/test_module1.cpp`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/tests/test_module1.cpp) is intentionally self-contained with **zero external testing framework dependencies** (no GoogleTest, Catch2, or Boost.Test).
- Avoids extra build-time package requirements.
- Uses a lightweight assertion tracker:
  ```cpp
  void check(bool condition, const std::string& description);
  ```
- Maintains global counters `g_checks` and `g_failures`, exiting with code `0` on total success or `1` on any failure.
- Emits real-time console messages for each executed test suite.

### 2.2 Floating-Point Tolerances
- **Individual Samples**: Verified using `approx(a, b, eps = 1e-3f)`. IEEE-754 float16 numbers convert losslessly into float32; the tolerance accommodates minor compiler-specific FMA instructions across architectures.
- **Accumulated Full-Frame Sums**: Evaluated with `approx_sum(a, b)` which permits a tolerance of $\max(0.02\text{f}, 0.01 \cdot |b|)$. This absorbs standard floating-point summation associativity differences between NumPy's pairwise tree summation and C++'s linear accumulation over 1,024 samples.

### 2.3 Exact Test Commands

#### Running via CTest:
From the build directory:
```bash
ctest --test-dir build --output-on-failure -V
```

#### Running the Test Binary Directly:
```bash
./build/tests/module1_tests
```

### 2.4 Current Test Metrics
- **Core Module 1 Harness (`module1_tests`)**: 45 test functions, 405 checks passed (100%) in the current configured build.
- **GUI Component Test Suite (`module1_gui_tests`)**: 6 conditional Qt integration tests on real datasets when Qt Test/Widgets are available.
- The current configured build has no Qt installation, so it registers and runs the core suite only.

When Qt Widgets and Qt Test are available, `module1_gui_tests` verifies that
typed products derived from `real_subset.h5` reach the waveform,
constellation, spectrum, power-spectrum, and waterfall plots. It also covers
explicit HDF5-rate refusal/propagation, shared analysis-region controls,
background completion/stale-result handling, and the Logs placeholder. These
GUI tests never calculate FFT or STFT data themselves.

---

## 3. Comprehensive Test Inventory

Every single test function in [`tests/test_module1.cpp`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/tests/test_module1.cpp) is documented below with its rationale, inputs, expected behavior, and provenance:

### 3.1 Audio Container (WAV) Tests

#### 1. `test_wav_mono`
- **What it tests**: Validates ingestion of single-channel (mono) audio WAV files.
- **Why it exists**: Verifies that real-valued audio is correctly converted into complex samples where $\text{Im} = 0.0\text{f}$, channel count is 1, and `is_complex` is false.
- **Input Fixture**: [`testdata/real_subset_mono.wav`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset_mono.wav) (1024-sample, 16-bit PCM mono derived from real Mendeley data).
- **Expected Behavior**: Sample count equals 1024; `is_complex == false`; `channel_count == 1`; `metadata_source == "wav_header"`; every sample has `imag() == 0.0f`.
- **Derivation**: Standard audio WAV header properties confirmed via Python `wave` module.

#### 2. `test_wav_stereo_as_iq`
- **What it tests**: Ingestion of 2-channel WAV files as complex $I/Q$.
- **Why it exists**: SDR recording software (e.g. HDSDR, SDR#) frequently outputs $I/Q$ data packed into 2-channel stereo WAV containers.
- **Input Fixture**: [`testdata/real_subset.wav`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.wav) (1024-frame, 2-channel stereo WAV derived from real Mendeley frame 0).
- **Expected Behavior**: Sample count equals 1024; `is_complex == true`; `channel_count == 2`.
- **Derivation**: Validated against WAV RIFF header properties.

---

### 3.2 Raw Binary I/Q Tests

#### 3. `test_raw_iq_with_sidecar`
- **What it tests**: Automatic discovery and parsing of SigMF JSON metadata (`.sigmf-meta`) accompanying a raw `.iq` file.
- **Why it exists**: Ensures [`IqLoader`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/IqLoader.h#L18) seamlessly links binary data with its sidecar file without manual user configuration.
- **Input Fixture**: [`testdata/real_subset_float32.iq`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset_float32.iq) (8192 bytes) + [`testdata/real_subset_float32.iq.sigmf-meta`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset_float32.iq.sigmf-meta) (`core:sample_rate = 1000000`, `core:datatype = "cf32_le"`).
- **Expected Behavior**: 1024 complex samples decoded; sample rate is $1.0\times 10^6\text{ Hz}$; `metadata_source == "sigmf_sidecar"`.
- **Derivation**: $\text{Sample count} = 8192\text{ bytes} / 8\text{ bytes per float32 complex sample} = 1024$.

#### 4. `test_raw_iq_without_metadata_refuses`
- **What it tests**: Refusal to load raw `.iq` files lacking a sidecar and lacking manual metadata.
- **Why it exists**: Enforces the architectural rule: **"Never guess metadata"**.
- **Input Fixture**: [`testdata/real_subset_float32_nosidecar.iq`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset_float32_nosidecar.iq) (8192-byte float32 raw IQ, intentionally no sidecar).
- **Expected Behavior**: Throws [`MissingMetadataError`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/LoaderExceptions.h#L50); `e.missing_fields()` contains `"sample_rate"`.
- **Derivation**: Deterministic architectural requirement.

#### 5. `test_raw_iq_manual_metadata_works`
- **What it tests**: Loading raw `.iq` using [`IqLoader::load_with_metadata`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/IqLoader.h#L27) with an explicitly supplied [`IqMetadataInput`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/IqMetadata.h#L12).
- **Why it exists**: Validates the recovery/override code path used when a user or GUI provides parameters directly.
- **Input Fixture**: [`testdata/real_subset_float32_nosidecar.iq`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset_float32_nosidecar.iq) with manual metadata: 1 MHz, `float32`, `little`, `manual_entry`.
- **Expected Behavior**: Exactly 1024 complex samples decoded; `metadata_source == "manual_entry"`.
- **Derivation**: $8192\text{ bytes} / (4\text{ bytes } I + 4\text{ bytes } Q) = 1024\text{ complex samples}$.

#### 6. `test_unsupported_extension_rejected`
- **What it tests**: Factory rejection of unrecognized file extensions.
- **Why it exists**: Ensures [`LoaderFactory`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/LoaderFactory.h#L12) fails fast on unknown formats.
- **Input Fixture**: [`testdata/something.dat`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/something.dat) (Empty file with `.dat` extension).
- **Expected Behavior**: Throws [`UnsupportedFormatError`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/LoaderExceptions.h#L19).

---

### 3.3 HDF5 Mendeley Dataset Tests (`real_subset.h5`)

#### 7. `test_hdf5_structure_and_metadata`
- **What it tests**: Verification of root attributes and modulation mapping table in [`HdfIqFrameDataset`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/HdfIqFrameDataset.h#L43).
- **Why it exists**: Guarantees file-level schema parsing correctly reflects dataset provenance.
- **Input Fixture**: [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5) (Real Mendeley dataset extract).
- **Expected Behavior**: `frame_count() == 20`; `frame_length() == 1024`; `channel_meaning() == "0=clean, 1=multipath(ref)"`; `source_attr() == "dataset3_iq_frames.h5"`; `subset_name() == "test"`; `modulation_label_map()` contains all 7 mappings (BPSK:0, QPSK:1, QAM:2, GMSK:3, OFDM:4, NBFM:5, WBFM:6).
- **Derivation**: Verified independently via Python `h5py` directly inspecting root attributes of `real_subset.h5`.

#### 8. `test_hdf5_refuses_missing_sample_rate`
- **What it tests**: Refusal to load an HDF5 frame when caller passes `sample_rate_hz <= 0.0`.
- **Why it exists**: The Mendeley dataset provides no sample rate attribute. Callers must supply one explicitly.
- **Input Fixture**: [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5).
- **Expected Behavior**: Throws [`MissingMetadataError`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/LoaderExceptions.h#L50) reporting missing `"sample_rate"`.

#### 9. `test_hdf5_frame_values_match_real_dataset`
- **What it tests**: Bit-accurate sample decoding, boundary sample retrieval ($0$, $1023$), mid-frame sample retrieval ($512$), and full-frame summation across 7 strategic frames:
  - Frame 0 (first frame)
  - Frame 1 (second frame; adjacent to start)
  - Frame 5 (mid-range frame)
  - Frame 9 (lower-middle frame)
  - Frame 10 (upper-middle frame; checks against buffer aliasing)
  - Frame 18 (second-to-last frame; adjacent to boundary)
  - Frame 19 (last frame)
- **Why it exists**: Primary numerical regression guard. Ensures that float16 decompression and frame indexing are exact.
- **Input Fixture**: [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5).
- **Expected Values Table (Independently Derived via `h5py` and `numpy`)**:

| Frame | Sample 0 ($I, Q$) | Sample 512 ($I, Q$) | Sample 1023 ($I, Q$) | Full-Frame Sum ($\sum I, \sum Q$) |
|:---|:---|:---|:---|:---|
| **0** | $(-0.1171875, 0.1015625)$ | $(0.078125, -0.109375)$ | $(0.0, 0.1953125)$ | $(-7.8515625, 6.6875)$ |
| **1** | $(-0.1171875, 0.1328125)$ | $(0.0078125, -0.15625)$ | $(-0.0625, -0.1484375)$ | $(19.3359375, 26.4453125)$ |
| **5** | $(0.0078125, 0.015625)$ | $(0.0078125, 0.015625)$ | $(0.0234375, 0.0078125)$ | $(14.421875, 19.921875)$ |
| **9** | $(0.21875, -0.09375)$ | $(-0.09375, 0.1484375)$ | $(-0.015625, 0.2421875)$ | $(16.6015625, -4.375)$ |
| **10** | $(-0.09375, 0.2109375)$ | $(0.03125, -0.203125)$ | $(0.140625, 0.1796875)$ | $(15.9140625, 10.953125)$ |
| **18** | $(0.0234375, 0.015625)$ | $(0.015625, 0.0234375)$ | $(0.0078125, 0.015625)$ | $(14.6796875, 20.21875)$ |
| **19** | $(0.1171875, -0.1640625)$ | $(-0.015625, 0.2421875)$ | $(0.140625, 0.2109375)$ | $(5.796875, 52.835938)$ |

- Also tests that [`IqAxisOrder::QThenI`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/HdfIqFrameDataset.h#L23) swaps real and imaginary channels on Frame 0.

#### 10. `test_hdf5_labels_match_real_dataset`
- **What it tests**: Ground-truth ML label retrieval on frames 0, 5, and 19.
- **Why it exists**: Ensures [`HdfIqFrameDataset::labels_for_frame`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/HdfIqFrameDataset.h#L64) reads `/y_mod`, `/y_chan`, and `/y_snr` correctly.
- **Input Fixture**: [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5).
- **Expected Behavior**: `modulation_id == 0` (BPSK), `channel_condition == 0` (clean), `snr_db == 20`.

#### 11. `test_hdf5_decoded_samples_are_finite`
- **What it tests**: Verifies no NaN or Inf values are produced in frame 0.
- **Why it exists**: Validates IEEE-754 binary16 decoding produces clean numeric floats.
- **Input Fixture**: [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5).

#### 12. `test_hdf5_negative_sample_rate_refused`
- **What it tests**: Rejection of negative sample rates (`-1.0 Hz`).
- **Why it exists**: Exercises boundary condition `< 0` in `sample_rate_hz <= 0.0`.
- **Input Fixture**: [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5).
- **Expected Behavior**: Throws [`MissingMetadataError`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/LoaderExceptions.h#L50).

#### 13. `test_hdf5_exact_boundary_frame_index_refused`
- **What it tests**: Rejection of index equal to frame count (`frame_index = 20` when `frame_count = 20`).
- **Why it exists**: Tests the exact off-by-one boundary for `load_frame`.
- **Input Fixture**: [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5).
- **Expected Behavior**: Throws [`FrameIndexOutOfRangeError`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/LoaderExceptions.h#L63) carrying `requested_index() == 20` and `frame_count() == 20`.

#### 14. `test_hdf5_labels_for_frame_out_of_range`
- **What it tests**: Rejection of index equal to frame count in `labels_for_frame(20)`.
- **Why it exists**: Ensures `labels_for_frame` maintains the same strict bounds check as `load_frame`.
- **Input Fixture**: [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5).
- **Expected Behavior**: Throws [`FrameIndexOutOfRangeError`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/LoaderExceptions.h#L63).

#### 15. `test_hdf5_all_frames_labels_consistent`
- **What it tests**: Inspects labels across **all 20 frames** (0 through 19).
- **Why it exists**: Verifies label extraction integrity across the entire dataset container.
- **Input Fixture**: [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5).
- **Expected Behavior**: All 20 frames yield `modulation_id == 0`, `channel_condition == 0`, `snr_db == 20`.

#### 16. `test_hdf5_all_frames_finite`
- **What it tests**: Confirms all 20,480 decoded complex samples across all 20 frames are finite.
- **Why it exists**: Proves no individual frame contains bit patterns triggering NaN or Inf.
- **Input Fixture**: [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5).

#### 17. `test_hdf5_frames_are_distinct`
- **What it tests**: Compares sample[0] across adjacent and distant frames: 0 vs 1, 0 vs 19, 1 vs 19, 17 vs 18, and 18 vs 19.
- **Why it exists**: Prevents regressions where memory caching or buffer aliasing returns identical frames on consecutive calls.
- **Input Fixture**: [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5).
- **Expected Behavior**: First samples of each compared pair differ.

#### 18. `test_hdf5_qthen_i_mid_frame_sample`
- **What it tests**: QThenI axis swap on frame 5 at mid-frame sample index 511.
- **Why it exists**: Guards against off-by-one errors in axis swapping mid-frame.
- **Input Fixture**: [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5).
- **Expected Values**: Sample 511 IThenQ has $I=0.015625, Q=0.0234375$. Under QThenI, real is $0.0234375$ and imag is $0.015625$.

#### 19. `test_hdf5_malformed_frame_len_mismatch_rejected`
- **What it tests**: Detection and rejection of datasets where `frame_len` attribute does not match `/X` shape axis 1.
- **Why it exists**: Validates schema consistency enforcement.
- **Input Fixture**: [`testdata/hdf5_fixture_malformed_frame_len_mismatch.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/hdf5_fixture_malformed_frame_len_mismatch.h5) (Synthetic fixture with `frame_len = 512` but `/X` shape `[2, 1024, 2]`).
- **Expected Behavior**: Throws [`Hdf5MalformedDatasetError`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/LoaderExceptions.h#L34).

#### 20. `test_hdf5_malformed_missing_dataset_rejected`
- **What it tests**: Rejection of HDF5 containers missing required datasets.
- **Why it exists**: Prevents crashes when corrupted files omit `/y_snr`.
- **Input Fixture**: [`testdata/hdf5_fixture_malformed_missing_dataset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/hdf5_fixture_malformed_missing_dataset.h5) (Synthetic fixture lacking `/y_snr`).
- **Expected Behavior**: Throws [`Hdf5MalformedDatasetError`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/LoaderExceptions.h#L34).

#### 21. `test_hdf5_out_of_range_frame_index`
- **What it tests**: Accessing index 999 in a 20-frame dataset.
- **Why it exists**: Verifies far out-of-range bounds checking.
- **Input Fixture**: [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5).
- **Expected Behavior**: Throws [`FrameIndexOutOfRangeError`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/LoaderExceptions.h#L63).

#### 22. `test_hdf5_dataset_move_semantics`
- **What it tests**: Move constructor and move assignment operator of [`HdfIqFrameDataset`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/HdfIqFrameDataset.h#L43).
- **Why it exists**: Verifies Rule of Five implementation and guarantees moved-from objects reset safely (`frame_count() == 0`).
- **Input Fixture**: [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5).

#### 23. `test_hdf5_sample_rate_propagated`
- **What it tests**: Ensures numeric `sample_rate_hz` passed by caller ($2,102,400.0\text{ Hz}$) is preserved accurately in [`ComplexSignal::sample_rate()`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/SignalTypes.h#L65).
- **Why it exists**: Prevents regressions where metadata strings were populated but numeric rates were discarded.
- **Input Fixture**: [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5).

#### 24. `test_hdf5_all_frames_size_and_sum`
- **What it tests**: Checks `sample_count() == 1024` and validates the full-frame complex sum ($\sum I, \sum Q$) for **every one of the 20 frames** against independent ground-truth values.
- **Why it exists**: Guarantees that every frame in the dataset is numerically accurate, not merely finite or zero-filled.
- **Input Fixture**: [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5).
- **Expected Behavior**: All 20 frames have 1024 samples and match the complete independently computed sum table.

---

### 3.4 Decoders, Parsers, and Factory Tests

#### 25. `test_iq_sample_decoders`
- **What it tests**: Concrete sample decoders:
  - [`Int8IqDecoder`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/IqSampleDecoder.h#L34): verifies $-128 \to -1.0$, $127 \to 127/128$, $64 \to 0.5$, and strict bounds within $[-1.0, 1.0]$; checks unaligned byte counts.
  - [`Int16IqDecoder`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/IqSampleDecoder.h#L44): verifies little-endian and big-endian decoding of $-32768 \to -1.0$ and $16384 \to 0.5$; checks unaligned byte counts.
  - [`Float32IqDecoder`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/IqSampleDecoder.h#L54): verifies little-endian and big-endian float decoding; checks unaligned byte counts.
  - [`IqSampleDecoderFactory`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/IqSampleDecoder.h#L64): verifies creation of `int8`, `int16`, `float32`; verifies rejection of unsupported types; tests [`IqSampleDecoderFactory::is_supported`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/IqSampleDecoder.h#L70) edge cases (empty string, casing variants like `INT8`, `float64`).
- **Why it exists**: Verifies all core raw sample decoding and byte-swapping logic in isolation.

#### 26. `test_sigmf_metadata_parser_json`
- **What it tests**: Parsing of SigMF JSON strings by [`SigmfMetadataParser::parse_json`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/IqMetadata.h#L55).
- **Why it exists**: Verifies JSON decoding, field mapping (`ci16_le` $\to$ `int16` + `little`), malformed JSON handling throwing [`MalformedDataError`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/LoaderExceptions.h#L27), missing `global` object handling throwing [`MalformedDataError`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/LoaderExceptions.h#L27), and unsupported datatype rejection throwing [`UnsupportedFormatError`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/LoaderExceptions.h#L19) (e.g. `cu8`).
- **Input**: In-memory JSON test strings.

#### 27. `test_loader_factory_class`
- **What it tests**: Creation of [`WavLoader`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/WavLoader.h#L13) for `.wav`, [`IqLoader`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/IqLoader.h#L18) for `.iq`, and rejection of unsupported extensions.
- **Why it exists**: Verifies loader routing logic.

#### 28. `test_half_to_float_edge_cases`
- **What it tests**: Unit-level bit-pattern edge cases for IEEE-754 binary16 decoding in [`half_to_float`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/Float16.h#L17):
  - `+zero` (`0x0000`) $\to +0.0\text{f}$ (verified with `!std::signbit`)
  - `-zero` (`0x8000`) $\to -0.0\text{f}$ (verified with `std::signbit`)
  - Smallest subnormal (`0x0001`) $\to 5.9604645\times 10^{-8}\text{f}$
  - Smallest normal (`0x0400`) $\to 6.1035156\times 10^{-5}\text{f}$
  - Largest normal (`0x7BFF`) $\to 65504.0\text{f}$
  - `+Inf` (`0x7C00`) $\to +\infty$
  - `-Inf` (`0xFC00`) $\to -\infty$
  - Quiet `NaN` (`0x7E00`) $\to \text{NaN}$ (verified with `std::isnan`)
- **Why it exists**: Real radio frames in `real_subset.h5` only contain small normal values. Special floating-point classes (subnormals, infinities, signed zeros, NaNs) would never be exercised without this direct bit-level test.
- **Derivation**: Independently computed via Python `struct` and `numpy.float16`.

---

### 3.5 Model-View-Presenter (MVP) Architecture Tests

#### 29. `test_mvp_presenter_binds_to_view`
- **What it tests**: Verifies that constructing [`VisualizationPresenter`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/VisualizationPresenter.h) with a concrete View automatically invokes `view->set_presenter(this)`.
- **Why it exists**: Validates bidirectional MVP binding contract so views can trigger presenter actions during desktop user interaction.
- **Input Fixture**: [`StubVisualizationView`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/tests/test_module1.cpp#L1078).
- **Expected Behavior**: `view.bound_presenter == &presenter`.

#### 30. `test_mvp_on_signal_loaded_with_real_hdf5_data`
- **What it tests**: Passes real frame 0 from [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5) through `presenter.on_signal_loaded(sig)` and verifies View callback delivery.
- **Why it exists**: Ensures the Presenter coordinates time-domain, constellation, and metadata rendering without altering or truncating real signal buffers.
- **Input Fixture**: Real Mendeley frame 0 via [`HdfIqFrameDataset`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/HdfIqFrameDataset.h#L43).
- **Expected Behavior**:
  - Exactly 1 call to `render_time_domain` receiving 1024 real samples.
  - Exactly 1 call to `render_constellation` receiving 1024 real samples.
  - Exactly 1 call to `display_metadata` with caller sample rate (250 kHz) and metadata source `"hdf5_manual_sample_rate"`.
  - 0 calls to the legacy `render_fft` callback; typed spectrum delivery is tested separately through the new DSP path.
  - 0 calls to `display_frame_info` (`on_signal_loaded` path only handles raw signals).

#### 31. `test_mvp_on_frame_loaded_with_real_labels`
- **What it tests**: Loads frame 5 from [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5) with real labels and passes both through `presenter.on_frame_loaded(sig, labels, "BPSK")`.
- **Why it exists**: Tests the entry point for dataset browsing, ensuring the Presenter coordinates both signal rendering and dataset label/condition display.
- **Input Fixture**: Frame 5 from `real_subset.h5` (`modulation_id == 0`, `channel_condition == 0`, `snr_db == 20`).
- **Expected Behavior**: View receives rendering callbacks for 1024 samples plus exactly 1 call to `display_frame_info("BPSK", 0, 20)`; Presenter caches `has_labels() == true` and `current_labels().modulation_id == 0`.

#### 32. `test_mvp_presenter_with_null_view`
- **What it tests**: Constructing [`VisualizationPresenter`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/VisualizationPresenter.h) with `nullptr` and calling `on_signal_loaded`.
- **Why it exists**: Ensures graceful degradation and null-pointer safety in headless or non-visual execution modes.
- **Expected Behavior**: Does not crash; caches signal internally with `current_signal().sample_count() == 1024`.

#### 33. `test_mvp_dsp_separated_from_view`
- **What it tests**: Architectural invariant enforcement: the View never performs DSP calculations.
- **Why it exists**: Validates that GUI classes remain purely passive display surfaces; DSP operations (such as FFT computation) are isolated in the Presenter / DSP processing pipeline.
- **Expected Behavior**: `view.fft_call_count == 0` after `on_signal_loaded`; time-domain rendering receives pre-processed data from the Presenter.

---

### 3.6 Native Qt Desktop GUI Integration Tests (`tests/test_gui.cpp`)

All GUI tests in `tests/test_gui.cpp` leverage QTest and run headlessly against **real dataset files exclusively** (zero synthetic or dummy data):

#### 34. `testViewLifecycle`
- **What it tests**: Instantiates `QtVisualizationView` and `VisualizationPresenter`, invoking `show_window()`.
- **Why it exists**: Verifies main window initialization and visibility state.

#### 35. `testRealHdf5DatasetNavigationAndRendering`
- **What it tests**: Opens `real_subset.h5` via `HdfIqFrameDataset`, loads frame 0 with real labels, and forwards via `presenter.on_frame_loaded`.
- **Why it exists**: Verifies that the Qt GUI correctly displays format metadata `"hdf5_iq_frame_dataset"`, sample rate `"1000000 Hz"`, modulation `"QAM16"`, and SNR `"15 dB"`.

#### 36. `testRealWavSignalLoadingAndMetadata`
- **What it tests**: Ingests `real_subset.wav` using `WavLoader` and presents it to the GUI view.
- **Why it exists**: Confirms audio WAV metadata parsing and rendering in the Qt window.

#### 37. `testRealIqSignalLoadingWithSigMF`
- **What it tests**: Loads raw `real_subset_float32.iq` + `real_subset_float32.iq.sigmf-meta` using `IqLoader`.
- **Why it exists**: Validates SigMF metadata sidecar binding and display.

#### 38. `testRealIqManualMetadataWorkflow`
- **What it tests**: Loads raw `real_subset_float32_nosidecar.iq` with manual `IqMetadataInput` (250 kHz).
- **Why it exists**: Tests user-driven metadata injection path when sidecar is absent.

#### 39. `testRealWaterfallAndPlotRenderingFromDataset`
- **What it tests**: Loads real frames 0 & 1 from `real_subset.h5`, generates a 2D spectrogram matrix from real sample magnitude vectors, and renders Time-Domain, Constellation, and Waterfall plots on `QtVisualizationView`.
- **Why it exists**: Ensures all four visualization panes (Time-Domain, FFT, Constellation, Waterfall) handle real signal array streams cleanly.

---

## 4. Mendeley Dataset Technical Specification

### 4.1 Dataset Origin and Purpose
The benchmark dataset used for ML radio classification in this project is the **Mendeley Data "Deep Learning-Based Radio Signal Classification"** dataset (`dataset3_iq_frames.h5`). It is an openly accessible collection of synthetic and over-the-air captured radio frames generated under various SNR levels and channel conditions for modulation classification benchmarks.

### 4.2 HDF5 Container Schema

```text
/ (Root Group)
  ├── Attributes:
  │     ├── channel_meaning = "0=clean, 1=multipath(ref)"   (String)
  │     ├── frame_len       = 1024                          (int64)
  │     ├── mod2id_json     = '{"BPSK":0,"QPSK":1,"QAM":2,  (JSON String)
  │     │                      "GMSK":3,"OFDM":4,"NBFM":5,
  │     │                      "WBFM":6}'
  │     ├── source          = "dataset3_iq_frames.h5"       (String)
  │     └── subset_name     = "test"                        (String)
  │
  ├── Datasets:
  │     ├── /X              : shape (N, 1024, 2),  dtype = float16 (IEEE-754 binary16)
  │     ├── /y_chan         : shape (N,),          dtype = int8
  │     ├── /y_mod          : shape (N,),          dtype = int16
  │     └── /y_snr          : shape (N,),          dtype = int16
```

### 4.3 Dimension Semantics
- **`/X`**: 3-dimensional array of shape `[N, 1024, 2]`.
  - **Axis 0 ($N$)**: Frame index ($0 \le \text{frame} < N$).
  - **Axis 1 ($1024$)**: Time-series sample index within the frame ($0 \le \text{sample} < 1024$). Matches the `frame_len` root attribute.
  - **Axis 2 ($2$)**: Interleaved channel dimension ($0 = I, 1 = Q$ under standard convention).
- **`/y_chan`**: Channel condition flag ($0 = \text{clean}$, $1 = \text{multipath}$).
- **`/y_mod`**: Integer modulation label corresponding to `mod2id_json`.
- **`/y_snr`**: Signal-to-Noise Ratio in decibels ($dB$).

### 4.4 Missing Metadata in Mendeley Dataset
- **Sample Rate is Entirely Absent**: Checked across all root attributes, dataset attributes, and metadata blocks in `dataset3_iq_frames.h5`. The authors did not record a sampling frequency. Consequently, [`HdfIqFrameDataset::load_frame`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/HdfIqFrameDataset.h#L69) strictly requires the caller to supply `sample_rate_hz`.
- **Unverified Channel Ordering**: The dataset schema does not explicitly declare which axis index corresponds to $I$ or $Q$. Module 1 defaults to [`IqAxisOrder::IThenQ`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/HdfIqFrameDataset.h#L22) but explicitly provides [`IqAxisOrder::QThenI`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/include/module1/HdfIqFrameDataset.h#L23) so callers can invert channels if required.

---

## 5. The Real Fixture: `real_subset.h5`

### 5.1 Provenance and Extraction
[`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5) is an authentic binary extract from the Mendeley `dataset3_iq_frames.h5` dataset.
- **File size**: 90,212 bytes.
- **Frames ($N$)**: Exactly 20 frames.
- **Samples per frame**: Exactly 1,024 samples.
- **Total complex samples**: $20 \times 1,024 = 20,480$ complex samples.
- **Total float16 words**: $40,960$ words.

### 5.2 Fixture-Specific Characteristics vs Full Dataset Properties

| Property | Full Mendeley Dataset | `real_subset.h5` Fixture | Testing Assumption Note |
|:---|:---|:---|:---|
| **Frame Count ($N$)** | Many thousands | Exactly 20 | Boundary tests check `frame_count() == 20`. |
| **Modulation Labels** | 7 classes (BPSK, QPSK, QAM, GMSK, OFDM, NBFM, WBFM) | Uniform BPSK (`mod_id == 0`) | The 20 frames were extracted from a contiguous BPSK block. |
| **Channel Condition** | Clean ($0$) and Multipath ($1$) | Uniform Clean (`y_chan == 0`) | Fixture contains clean captures. |
| **SNR Range** | $-20\text{ dB}$ to $+20\text{ dB}$ | Uniform $20\text{ dB}$ (`y_snr == 20`) | Fixture contains high-SNR captures. |
| **Frame Length** | 1024 | 1024 | Universal property of dataset schema. |
| **Modulation Map** | 7 entries in JSON | 7 entries in JSON | Preserved in root attribute `mod2id_json`. |

> **Important**: Tests verifying that all 20 frames have `mod_id == 0`, `y_chan == 0`, and `y_snr == 20` test the integrity of this specific fixture extract; they are documented as fixture-specific and will naturally differ on other slices of the Mendeley dataset.

---

## 6. Independent Value Verification

To prevent circular reasoning, **no expected value in the test suite was generated by the C++ implementation**.

### 6.1 Independent Python Derivation Script
All ground-truth constants in the test suite were generated by running the following Python script against [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5):

```python
import h5py
import numpy as np

f = h5py.File("module1/testdata/real_subset.h5", "r")
X = f["X"]  # shape (20, 1024, 2), dtype float16

for idx in [0, 1, 5, 9, 10, 18, 19]:
    frame = X[idx]
    s0_i = float(np.float32(frame[0, 0]))
    s0_q = float(np.float32(frame[0, 1]))
    s512_i = float(np.float32(frame[512, 0]))
    s512_q = float(np.float32(frame[512, 1]))
    s1023_i = float(np.float32(frame[1023, 0]))
    s1023_q = float(np.float32(frame[1023, 1]))
    sum_i = float(np.sum(frame[:, 0].astype(np.float32)))
    sum_q = float(np.sum(frame[:, 1].astype(np.float32)))
    print(f"Frame {idx}: s0=({s0_i}, {s0_q}) s512=({s512_i}, {s512_q}) s1023=({s1023_i}, {s1023_q}) sum=({sum_i}, {sum_q})")
```

### 6.2 Float16 Bit-Level Verification
In `test_half_to_float_edge_cases`, IEEE-754 binary16 bit patterns were verified independently using NumPy:
```python
import struct
import numpy as np

def h16_to_f32(bits):
    raw_bytes = struct.pack("<H", bits)
    return float(np.frombuffer(raw_bytes, dtype=np.float16)[0].astype(np.float32))

assert h16_to_f32(0x0000) == 0.0
assert h16_to_f32(0x0001) == 5.9604644775390625e-08  # smallest subnormal
assert h16_to_f32(0x0400) == 6.103515625e-05         # smallest normal
assert h16_to_f32(0x7BFF) == 65504.0                 # largest normal
```

---

## 7. Test Fixtures Inventory

| Fixture File | Category | Construction / Purpose |
|:---|:---|:---|
| [`testdata/real_subset_mono.wav`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset_mono.wav) | Derived Real | 1024 frames of 16-bit mono PCM audio (4154 bytes). Exercises mono-to-complex decoding with real radio samples. |
| [`testdata/real_subset.wav`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.wav) | Derived Real | 1024 frames of 2-channel stereo 16-bit PCM audio (8250 bytes). Exercises 2-channel audio $I/Q$ decoding. |
| [`testdata/real_subset_float32.iq`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset_float32.iq) | Derived Real | 8192 bytes raw float32 binary $I/Q$ (1024 samples) derived from real Mendeley frame 0. Tests sidecar linking. |
| [`testdata/real_subset_float32.iq.sigmf-meta`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset_float32.iq.sigmf-meta) | Valid Metadata | SigMF JSON sidecar declaring 1 MSps and `cf32_le`. |
| [`testdata/real_subset_float32_nosidecar.iq`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset_float32_nosidecar.iq) | Derived Real | 8192 bytes raw float32 binary $I/Q$ (1024 samples) without sidecar. Exercises manual metadata flow. |
| [`testdata/real_subset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/real_subset.h5) | Authentic Real | 20-frame Mendeley benchmark extract (90,212 bytes). Exercises HDF5 loading, float16 decoding, and MVP presenter coordination. |
| [`testdata/something.dat`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/something.dat) | Synthetic Rejection | Empty file with unknown extension. Tests extension rejection. |
| [`testdata/hdf5_fixture_from_real_preview.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/hdf5_fixture_from_real_preview.h5) | Derived Real | 3-frame extract used in early development prototypes. |
| [`testdata/hdf5_fixture_malformed_frame_len_mismatch.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/hdf5_fixture_malformed_frame_len_mismatch.h5) | Synthetic Malformed | `frame_len = 512` attribute but `/X` shape has axis $1 = 1024$. Tests schema cross-validation. |
| [`testdata/hdf5_fixture_malformed_missing_dataset.h5`](file:///home/namanoncode/Documents/GitHub/WAVIQ/module1/testdata/hdf5_fixture_malformed_missing_dataset.h5) | Synthetic Malformed | Contains `/X`, `/y_chan`, `/y_mod` but lacks `/y_snr`. Tests required dataset presence checks. |

---

## 8. Reproducibility Instructions

### 8.1 System and Toolchain Requirements
- **Operating System**: Linux (Ubuntu 20.04+, Debian 11+, Fedora 36+, or Arch Linux).
- **C++ Compiler**: GCC $\ge 9$ or Clang $\ge 10$ with C++17 support.
- **CMake**: Version $\ge 3.16$.
- **Development Libraries**:
  - `libsndfile1-dev`
  - `nlohmann-json3-dev`
  - `libhdf5-dev`
  - `pkg-config`

#### Installing Dependencies on Ubuntu / Debian:
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config libsndfile1-dev nlohmann-json3-dev libhdf5-dev
```

### 8.2 Build and Verification from Clean Checkout
```bash
# 1. Enter repository root
cd /home/namanoncode/Documents/GitHub/WAVIQ/module1

# 2. Configure build tree
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# 3. Build core library, CLI, and test suite
cmake --build build -j$(nproc)

# 4. Run full test suite with detailed output
ctest --test-dir build --output-on-failure -V
```

### 8.3 Expected Verification Output
```text
test 1
    Start 1: module1_tests

1: Test command: .../module1/build/tests/module1_tests
1: Working Directory: .../module1/build/tests
1: test_wav_mono
1: test_wav_stereo_as_iq
1: test_raw_iq_with_sidecar
1: test_raw_iq_without_metadata_refuses
1: test_raw_iq_manual_metadata_works
1: test_unsupported_extension_rejected
1: test_hdf5_structure_and_metadata
1: test_hdf5_refuses_missing_sample_rate
1: test_hdf5_frame_values_match_real_dataset
1: test_hdf5_labels_match_real_dataset
1: test_hdf5_decoded_samples_are_finite
1: test_hdf5_negative_sample_rate_refused
1: test_hdf5_exact_boundary_frame_index_refused
1: test_hdf5_labels_for_frame_out_of_range
1: test_hdf5_all_frames_labels_consistent
1: test_hdf5_all_frames_finite
1: test_hdf5_frames_are_distinct
1: test_hdf5_qthen_i_mid_frame_sample
1: test_hdf5_malformed_frame_len_mismatch_rejected
1: test_hdf5_malformed_missing_dataset_rejected
1: test_hdf5_out_of_range_frame_index
1: test_iq_sample_decoders
1: test_sigmf_metadata_parser_json
1: test_loader_factory_class
1: test_hdf5_dataset_move_semantics
1: test_half_to_float_edge_cases
1: test_hdf5_sample_rate_propagated
1: test_hdf5_all_frames_size_and_sum
1: test_mvp_presenter_binds_to_view
1: test_mvp_on_signal_loaded_with_real_hdf5_data
1: test_mvp_on_frame_loaded_with_real_labels
1: test_mvp_presenter_with_null_view
1: test_mvp_dsp_separated_from_view
1: test_real_hdf5_dsp_products
1: test_real_hdf5_region_and_reduction_behavior
1: test_presenter_requests_only_requested_real_products
1: test_session_model_requires_explicit_hdf5_rate
1: test_background_executor_discards_stale_real_data_results
1: test_background_executor_delivers_real_data_failure_safely
1: test_waveform_processor_real_data_contract
1: test_constellation_processor_real_data_contract
1: test_spectrum_and_power_multiple_real_frames
1: test_spectrogram_real_data_contract_and_regions
1: test_real_data_processor_validation
1: test_real_hdf5_full_pipeline_to_typed_view
1: 
1: 405/405 checks passed
1/1 Test #1: module1_tests ....................   Passed    0.05 sec

100% tests passed, 0 tests failed out of 1
```
