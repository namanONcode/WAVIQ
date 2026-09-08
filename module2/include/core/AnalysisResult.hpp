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
    float amplitudeMean;
    float amplitudeVar;
    float phaseMean;
    float phaseVar;
    float freqMean;
    float freqVar;
    
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
