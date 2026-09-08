# Module 2: Signal Analysis Engine Walkthrough

This document outlines the completion of all milestones (M1 to M10) for **Module 2** (Signal Analysis Engine) of the WAVIQ project.

## Overview
Module 2 is responsible for taking baseband IQ samples and translating them into meaningful analysis (SNR, bandwidth, symbol rate), classifying the modulation scheme, synchronizing the signal, and outputting demodulated soft bits (LLRs) for Forward Error Correction (FEC) in Module 3.

## Milestone Status

- **M1: C++/CMake foundation & Core Data Structures**: Created project structure, CMake files, `SignalData`, and `AnalysisResult`.
- **M2: Signal Preprocessing**: Implemented DC removal and normalization algorithms.
- **M3: FFT/PSD**: Implemented windowing and spectrum analysis via FFTW3 wrappers.
- **M4: Parameter extraction**: Created estimators for carrier frequency, bandwidth, and SNR.
- **M5: Symbol-rate estimation**: Implemented non-linear processing to detect cyclostationary features.
- **M6: Feature extraction**: Implemented statistical algorithms (Amplitude, Phase, Frequency, Cumulant features) and integrated them into a unified `FeatureExtractor`.
- **M7: Modulation classifier**: Implemented a rule-based `DSPClassifier` to differentiate between FSK, PSK, and QAM. Created a `ModulationClassifier` wrapper for future ML integration.
- **M8: Synchronization**: Set up `MatchedFilter`, `CarrierRecovery` (Costas Loop), and `TimingRecovery` (Gardner TED).
- **M9: Demodulators**: Implemented soft-decision (LLR) demodulators (`PSKDemodulator`, `FSKDemodulator`, `QAMDemodulator`). These conform strictly to outputting `SoftBitStreamFloat` containing `std::vector<float>` as required by the Module 3 interface.
- **M10: Integration**: Built `AnalysisOrchestrator` to string together M6 through M9 into a single cohesive pipeline capable of taking raw `SignalData` and returning a populated `AnalysisResult`.

## Compatibility with Module 1 (Codebase Verification)

A codebase analysis of Module 1 and Module 2 shows seamless structural compatibility. 
- **Module 1 Output (`module1::ComplexSignal`)**: Contains `std::vector<std::complex<float>> samples()` and a `double sample_rate()`.
- **Module 2 Input (`module2::core::SignalData`)**: Expects `std::vector<std::complex<float>> samples` and `double sampleRate`.

Integration is straightforward. A top-level application merely needs to pass the sample vector and sample rate from Module 1's loader directly into Module 2's `AnalysisOrchestrator::analyze(SignalData)` method.

> [!TIP]
> With M10 complete, Module 2 is structurally complete and ready for integration with Module 3 (FEC & Decode). Ensure that Module 3 is prepared to accept `module2::core::SoftBitStreamFloat`!
