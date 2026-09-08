#include "features/FeatureExtractor.hpp"
#include "features/AmplitudeFeatures.hpp"
#include "features/PhaseFeatures.hpp"
#include "features/FrequencyFeatures.hpp"
#include "features/CumulantFeatures.hpp"

namespace module2 {
namespace features {

core::SignalFeatures FeatureExtractor::extractAll(const core::SignalData& data) {
    core::SignalFeatures sf = {};

    // Compute basic statistics
    auto ampStats = AmplitudeFeatures::compute(data);
    sf.amplitudeMean = ampStats.mean;
    sf.amplitudeVar = ampStats.variance;

    auto phaseStats = PhaseFeatures::compute(data);
    sf.phaseMean = phaseStats.mean;
    sf.phaseVar = phaseStats.variance;

    auto freqStats = FrequencyFeatures::compute(data);
    sf.freqMean = freqStats.mean;
    sf.freqVar = freqStats.variance;

    // Compute higher order cumulants
    auto cumulantVals = CumulantFeatures::compute(data);
    sf.cumulants = CumulantFeatures::toVector(cumulantVals);

    return sf;
}

} // namespace features
} // namespace module2
