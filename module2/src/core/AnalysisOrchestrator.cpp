#include "core/AnalysisOrchestrator.hpp"
#include "features/FeatureExtractor.hpp"
#include "classification/ModulationClassifier.hpp"
#include "synchronization/CarrierRecovery.hpp"
#include "synchronization/TimingRecovery.hpp"
#include "synchronization/MatchedFilter.hpp"
#include "demod/PSKDemodulator.hpp"
#include "demod/FSKDemodulator.hpp"
#include "demod/QAMDemodulator.hpp"

namespace module2 {
namespace core {

AnalysisResult AnalysisOrchestrator::analyze(SignalData data) {
    AnalysisResult result = {};

    // 1. Feature Extraction (M6)
    result.features = features::FeatureExtractor::extractAll(data);
    
    // 2. Modulation Classification (M7)
    result.modulation = classification::ModulationClassifier::classify(result.features);
    
    // 3. Synchronization (M8)
    auto carrierSynced = synchronization::CarrierRecovery::recover(data.samples);
    auto matched = synchronization::MatchedFilter::apply(carrierSynced);
    
    // Assuming 4 samples per symbol for this pipeline orchestration
    int samplesPerSymbol = 4;
    result.synchronizedSymbols = synchronization::TimingRecovery::recover(matched, samplesPerSymbol);

    // 4. Demodulation (M9) - Outputting Soft Decisions (LLRs)
    std::shared_ptr<demod::Demodulator> demodulator;
    if (result.modulation.modulationType == "FSK") {
        demodulator = std::make_shared<demod::FSKDemodulator>();
    } else if (result.modulation.modulationType == "QAM") {
        demodulator = std::make_shared<demod::QAMDemodulator>();
    } else {
        demodulator = std::make_shared<demod::PSKDemodulator>();
    }
    
    result.bitStream = demodulator->demodulate(result.synchronizedSymbols);
    
    // Additional parameter extraction (Fs, Fc, BW, SNR) would be integrated here
    // using FFTAnalyzer, CarrierEstimator, SNREstimator, etc.
    
    result.demodulation.success = true;
    result.demodulation.statusMessage = "Demodulated using " + result.modulation.modulationType;

    return result;
}

} // namespace core
} // namespace module2
