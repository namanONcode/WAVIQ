# Module 2: Signal Analysis Engine — Technical Specification

Provide a brief description of the problem, any background context, and what the change accomplishes.
This document translates the architectural plan for **Module 2** into a concrete C++ technical specification. It defines the folder structure, core classes, data structures, and algorithms for taking normalized complex IQ samples, analyzing them, and producing a reliable bitstream along with rich metadata for GUI visualization.

## User Review Required

> [!IMPORTANT]
> Please review this technical specification. It defines the exact C++ interfaces, data structures, algorithms, and project layout we will use. Once you approve this plan, we will proceed with creating the directory structure and implementing **Milestone 1** (C++/CMake foundation and Core Structures).

## Open Questions

> [!WARNING]
> 1. **Data Types**: The specification currently uses `std::complex<float>` for IQ samples to balance performance and precision. Let me know if you prefer `std::complex<double>` or a custom struct.
> 2. **ML Classification**: For the RadioML integration later on, should we expect ONNX models to be loaded via `onnxruntime-cxx` API natively inside Module 2, or will this be a separate service? (The current spec assumes linking `onnxruntime` in C++).

## 1. System Architecture & Folder Structure

The project will be initialized in `d:\sih2026\SignalAnalyzer` with the following clean boundary: Module 2 receives valid complex samples plus metadata, and returns signal parameters, modulation classification, synchronized symbols, demodulated bits, and confidence information.

```text
SignalAnalyzer/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── core/
│   │   ├── SignalParameters.hpp
│   │   ├── SignalFeatures.hpp
│   │   ├── BitStream.hpp
│   │   └── AnalysisResult.hpp
│   ├── preprocessing/
│   │   ├── DCRemoval.hpp
│   │   ├── Normalizer.hpp
│   │   ├── Filter.hpp
│   │   └── SignalDetector.hpp
│   ├── analysis/
│   │   ├── FFTAnalyzer.hpp
│   │   ├── SpectrumAnalyzer.hpp
│   │   ├── BandwidthEstimator.hpp
│   │   ├── CarrierEstimator.hpp
│   │   ├── SymbolRateEstimator.hpp
│   │   └── SNREstimator.hpp
│   ├── features/
│   │   ├── AmplitudeFeatures.hpp
│   │   ├── PhaseFeatures.hpp
│   │   ├── FrequencyFeatures.hpp
│   │   ├── SpectralFeatures.hpp
│   │   └── CumulantFeatures.hpp
│   ├── classification/
│   │   ├── ModulationClassifier.hpp
│   │   ├── DSPClassifier.hpp
│   │   └── MLClassifier.hpp
│   ├── synchronization/
│   │   ├── CarrierRecovery.hpp
│   │   ├── TimingRecovery.hpp
│   │   └── MatchedFilter.hpp
│   └── demod/
│       ├── Demodulator.hpp
│       ├── FSKDemodulator.hpp
│       ├── PSKDemodulator.hpp
│       └── QAMDemodulator.hpp
├── src/
│   ├── preprocessing/ ...
│   ├── analysis/ ...
│   ├── features/ ...
│   ├── classification/ ...
│   ├── synchronization/ ...
│   └── demod/ ...
└── tests/
    ├── test_preprocessing.cpp
    ├── test_fft.cpp
    └── ...
```

---

## 2. Core Data Structures (API Contracts)

These structures form the input/output boundaries of Module 2.

### [NEW] `include/core/SignalData.hpp`
```cpp
#pragma once
#include <vector>
#include <complex>

namespace module2 {
namespace core {

struct SignalData {
    std::vector<std::complex<float>> samples;
    double sampleRate;      // Fs in Hz
    double centerFrequency; // Fc in Hz
};

} // namespace core
} // namespace module2
```

### [NEW] `include/core/BitStream.hpp`
```cpp
#pragma once
#include <vector>
#include <cstdint>

namespace module2 {
namespace core {

enum class BitStreamType {
    HARD,
    SOFT_FLOAT,
    SOFT_INT8
};

/**
 * @brief Base class for demodulated bits.
 * Supports both hard decisions (0/1) and soft decisions (LLRs) for FEC in Module 3.
 */
class BitStream {
public:
    virtual ~BitStream() = default;
    virtual BitStreamType getType() const = 0;
    virtual size_t size() const = 0;
};

class HardBitStream : public BitStream {
public:
    std::vector<uint8_t> bits;
    
    BitStreamType getType() const override { return BitStreamType::HARD; }
    size_t size() const override { return bits.size(); }
};

class SoftBitStreamFloat : public BitStream {
public:
    std::vector<float> llrs; // Log-Likelihood Ratios
    
    BitStreamType getType() const override { return BitStreamType::SOFT_FLOAT; }
    size_t size() const override { return llrs.size(); }
};

class SoftBitStreamInt8 : public BitStream {
public:
    std::vector<int8_t> llrs;
    
    BitStreamType getType() const override { return BitStreamType::SOFT_INT8; }
    size_t size() const override { return llrs.size(); }
};

} // namespace core
} // namespace module2
```

### [NEW] `include/core/AnalysisResult.hpp`
```cpp
#pragma once
#include <vector>
#include <complex>
#include <string>
#include <memory>
#include "BitStream.hpp"

namespace module2 {
namespace core {

struct SignalParameters {
    double estimatedCarrierFreq; // Baseband carrier
    double occupiedBandwidth;
    double estimatedSymbolRate;
    double estimatedSNR;
    
    // Confidence scores (0.0 to 1.0)
    float carrierConfidence;
    float bandwidthConfidence;
    float symbolRateConfidence;
    float snrConfidence;
};

struct SignalFeatures {
    // Basic stats
    float amplitudeMean, amplitudeVar;
    float phaseMean, phaseVar;
    float freqMean, freqVar;
    
    // Higher order cumulants
    std::vector<float> cumulants; // e.g., C40, C42, C60
};

struct ModulationResult {
    std::string modulationType; // "FSK", "BPSK", "QPSK", "16QAM", etc.
    float confidence;
    bool isMLBased;
};

struct DemodulationResult {
    bool success;
    float evm;       // Error Vector Magnitude
    double ber;      // Bit Error Rate (if reference known, else -1.0)
    std::string statusMessage;
};

struct AnalysisResult {
    SignalParameters parameters;
    SignalFeatures features;
    ModulationResult modulation;
    
    // Visualization Data
    std::vector<float> spectrumPSD;
    std::vector<float> frequencyBins;
    
    // Demodulation Outputs
    std::vector<std::complex<float>> synchronizedSymbols;
    std::shared_ptr<BitStream> bitStream;
    
    DemodulationResult demodulation;
};

} // namespace core
} // namespace module2
```

---

## 3. Subsystem Specifications

### 3.1 Signal Preprocessing
**Goal:** Clean the signal and detect regions of interest.

*   **`DCRemoval::process(SignalData& data)`**: Calculates mean I and Q and subtracts them.
*   **`Normalizer::process(SignalData& data)`**: Divides all samples by the maximum absolute amplitude ($max |x[n]|$).
*   **`Filter::applyFIR(SignalData& data, const std::vector<float>& taps)`**: Standard FIR filtering.
*   **`SignalDetector::extractActiveRegion(const SignalData& data, float threshold)`**: Uses energy thresholding to strip noise tails.

### 3.2 Time & Frequency Analysis
**Goal:** Waveform analysis and FFT processing using FFTW3.

*   **`FFTAnalyzer`**: Wraps FFTW3. Applies windowing (Hann by default). Calculates PSD: $PSD[k] = |X[k]|^2$. Performs FFT shift so zero frequency is centered.
*   **`SpectrumAnalyzer`**: Computes peak frequency, spectral centroid, spectral bandwidth, and noise floor.

### 3.3 Parameter Extraction
**Goal:** Estimate Fs, Fc, BW, Rs, SNR.

*   **`CarrierEstimator`**: Uses peak spectral detection and phase derivative methods to find the carrier offset.
*   **`BandwidthEstimator`**: Implements 99% occupied bandwidth and -3dB thresholding over the PSD.
*   **`SNREstimator`**: Calculates signal power vs. noise floor (estimated from out-of-band spectral regions).
*   **`SymbolRateEstimator`**: Implements non-linear analysis (e.g., delay and multiply or squaring the signal) followed by FFT to detect cyclostationary features yielding candidate symbol rates.

### 3.4 Feature Extraction & Modulation Classification
**Goal:** Compute statistical features and classify the modulation.

*   **`FeatureExtractor`**: Computes amplitude, phase, frequency moments, and higher-order cumulants (C40, C42, C60).
*   **`DSPClassifier`**: Rule-based decision tree (e.g., variance of instantaneous frequency to separate FSK from PSK).
*   **`MLClassifier`**: Later integration point for RadioML ONNX models.
*   **`ModulationClassifier`**: Hybrid engine that takes inputs from `DSPClassifier` and `MLClassifier` to output a final `ModulationResult` with a confidence score.

### 3.5 Synchronization
**Goal:** Correct offsets and recover timing.

*   **`CarrierRecovery`**: Implements Costas Loop / PLL to drive carrier offset and phase offset to zero.
*   **`TimingRecovery`**: Implements Gardner Timing Error Detector (TED) with loop filter and interpolator.
*   **`MatchedFilter`**: Root Raised Cosine (RRC) filtering before timing recovery.

### 3.6 Demodulation Engine
**Goal:** Convert synchronized symbols to bitstream.

*   **`FSKDemodulator`**: Uses instantaneous frequency decisions.
*   **`PSKDemodulator`**: BPSK (sign decision), QPSK, 8PSK (phase region decisions).
*   **`QAMDemodulator`**: Nearest constellation point decision, EVM calculation, and Gray decoding.

---

## 4. CMake Setup & Dependencies

```cmake
cmake_minimum_required(VERSION 3.14)
project(SignalAnalyzer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(FFTW3 REQUIRED)

# Core Library
add_library(SignalAnalyzerEngine src/core/... src/preprocessing/... )
target_include_directories(SignalAnalyzerEngine PUBLIC include)
target_link_libraries(SignalAnalyzerEngine PRIVATE fftw3)

# Executable / Test Runner (Framework-Free)
add_executable(SignalAnalyzerTests tests/test_module2.cpp tests/test_preprocessing.cpp ...)
target_link_libraries(SignalAnalyzerTests PRIVATE SignalAnalyzerEngine)
```

---

## 5. Verification Plan

Following the strict testing principles established in Module 1, Module 2 testing will guarantee bit-accurate signal processing, zero silent data corruption, and mathematical correctness.

### 5.1 Framework-Free Test Architecture
Tests will be implemented with **zero external testing framework dependencies** (no GoogleTest, Catch2, etc.).
* We will use a lightweight assertion tracker with `g_checks` and `g_failures`, exiting with code `0` on success or `1` on failure.
* **Floating-Point Tolerances**: We will use custom `approx(a, b, eps)` and `approx_sum(a, b)` functions to compare floating-point values against expected ground-truth arrays, absorbing minor compiler/FMA associativity differences.

### 5.2 Independent Value Verification
To prevent circular reasoning, **no expected value in the test suite will be generated by the C++ implementation**.
* We will use independent Python scripts (NumPy, SciPy) to generate ground-truth fixtures and expected numeric outputs for filters, FFTs, feature extraction, and demodulation.
* These known inputs and expected outputs will be saved as binary fixtures (e.g. HDF5, raw IQ) or embedded directly in the test cases.

### 5.3 Automated Test Inventory
*   `test_preprocessing`: Verify DC removal, Normalization, and Filtering against SciPy ground truth.
*   `test_fft_and_psd`: Verify PSD peak, windowing, and spectral centroid match a known injected tone processed via NumPy.
*   `test_bandwidth`: Verify 99% OBW of a synthetic signal matches expected values.
*   `test_symbol_rate`: Verify the symbol rate estimator over different SNRs (-10dB to +20dB) against known symbols.
*   `test_demod_*`: End-to-end tests from IQ to Bits, asserting demodulated bits exactly match the known Python-generated sequence.

### 5.4 Manual Verification
Before closing M10, we will:
1. Provide a large chunked IQ file (e.g., using `IqLoader` from Module 1).
2. Run the `SignalAnalyzerEngine` CLI over it.
3. Validate memory consumption and processing time per 1M sample chunk.

---

## 6. Implementation Milestones (Roadmap)

We will execute this strictly in the following order:

*   [x] **M1:** C++/CMake foundation & Core Data Structures (`SignalData`, `AnalysisResult`).
*   [x] **M2:** Signal Preprocessing (DC removal, normalization).
*   [x] **M3:** FFT/PSD (FFTW3 integration, Windowing, Spectrum Analyzer).
*   [x] **M4:** Parameter extraction (Fs, Fc, BW, SNR).
*   [x] **M5:** Symbol-rate estimation (Non-linear processing).
*   [x] **M6:** Feature extraction (Stats & Cumulants).
*   [x] **M7:** Modulation classifier (Rule-based DSP initially).
*   [x] **M8:** Synchronization (Matched Filter, PLL, Gardner TED).
*   [x] **M9:** Demodulators (FSK, BPSK, QPSK → bits).
*   [ ] **M10:** Integration/Testing (QAM, chunked file processing, final API validation).
  