cmake_minimum_required(VERSION 3.14)
project(SignalAnalyzer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add include directory
include_directories(include)

# Use the manually downloaded FFTW3 directory
include_directories("D:/SIH2K26/WAVIQ/fftw3")

enable_testing()

# Core Library
# We will add more source files as we build them.
add_library(SignalAnalyzerEngine 
    src/preprocessing/DCRemoval.cpp
    src/preprocessing/Normalizer.cpp
    src/preprocessing/Filter.cpp
    src/preprocessing/SignalDetector.cpp
    src/core/AnalysisOrchestrator.cpp
    src/analysis/FFTAnalyzer.cpp
    src/analysis/SpectrumAnalyzer.cpp
    src/analysis/CarrierEstimator.cpp
    src/analysis/BandwidthEstimator.cpp
    src/analysis/SNREstimator.cpp
    src/analysis/SymbolRateEstimator.cpp
    src/features/AmplitudeFeatures.cpp
    src/features/CumulantFeatures.cpp
    src/features/FrequencyFeatures.cpp
    src/features/PhaseFeatures.cpp
    src/features/SpectralFeatures.cpp
    src/features/FeatureExtractor.cpp
    src/classification/DSPClassifier.cpp
    src/classification/ModulationClassifier.cpp
    src/synchronization/CarrierRecovery.cpp
    src/synchronization/TimingRecovery.cpp
    src/synchronization/MatchedFilter.cpp
    src/demod/PSKDemodulator.cpp
    src/demod/FSKDemodulator.cpp
    src/demod/QAMDemodulator.cpp
)
target_include_directories(SignalAnalyzerEngine PUBLIC include "D:/SIH2K26/WAVIQ/fftw3")
target_link_libraries(SignalAnalyzerEngine PRIVATE 
    "D:/SIH2K26/WAVIQ/fftw3/libfftw3-3.lib"
    "D:/SIH2K26/WAVIQ/fftw3/libfftw3f-3.lib"
)

# Test Executable
add_executable(SignalAnalyzerTests
    tests/DCRemovalTests.cpp
    tests/NormalizerTests.cpp
    tests/FilterTests.cpp
    tests/SignalDetectorTests.cpp
    tests/SymbolRateEstimatorTests.cpp
    tests/main.cpp
)
target_include_directories(SignalAnalyzerTests PRIVATE include)
target_link_libraries(SignalAnalyzerTests PRIVATE SignalAnalyzerEngine)
