#include "preprocessing/SignalDetector.hpp"
#include <cmath>

namespace module2 {
namespace preprocessing {

bool SignalDetector::extractActiveRegion(core::SignalData& data, float energyThreshold) {
    if (data.samples.empty()) {
        return false;
    }

    // Step 1: Calculate moving average energy to estimate noise floor and signal regions
    // For a robust implementation, we would use a windowed approach. 
    // Here we implement a simplified version.
    
    size_t numSamples = data.samples.size();
    std::vector<float> energy(numSamples);
    
    float totalEnergy = 0.0f;
    for (size_t i = 0; i < numSamples; ++i) {
        energy[i] = std::norm(data.samples[i]); // squared magnitude
        totalEnergy += energy[i];
    }
    
    // Simplistic noise floor estimation (assuming bottom 20% of energy is noise)
    // A better approach would sort and take the lower quartile, but this serves as a baseline.
    float averageEnergy = totalEnergy / numSamples;
    float threshold = averageEnergy * energyThreshold;

    size_t startIdx = 0;
    size_t endIdx = numSamples - 1;

    // Find start
    for (size_t i = 0; i < numSamples; ++i) {
        if (energy[i] > threshold) {
            startIdx = i;
            break;
        }
    }

    // Find end
    for (size_t i = numSamples - 1; i > startIdx; --i) {
        if (energy[i] > threshold) {
            endIdx = i;
            break;
        }
    }

    // Add some padding to avoid clipping the actual signal edges
    size_t padding = static_cast<size_t>(data.sampleRate * 0.001); // 1ms padding
    startIdx = (startIdx > padding) ? (startIdx - padding) : 0;
    endIdx = (endIdx + padding < numSamples) ? (endIdx + padding) : (numSamples - 1);

    if (startIdx >= endIdx || (endIdx - startIdx) < 100) { // Too small or invalid region
        return false;
    }

    // Extract the region
    std::vector<std::complex<float>> extracted(data.samples.begin() + startIdx, data.samples.begin() + endIdx + 1);
    data.samples = std::move(extracted);

    return true;
}

} // namespace preprocessing
} // namespace module2
