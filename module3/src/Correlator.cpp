#include "Correlator.hpp"
#include "Module3Exceptions.hpp"
#include <algorithm>
#include <cmath>

namespace module3 {

CorrelationResult SyncWordCorrelator::correlateAndExtract(const std::vector<uint8_t>& hardBits) {
    if (syncWord_.empty()) {
        throw CorrelatorError("Sync word cannot be empty.");
    }
    
    if (errorTolerance_ < 0 || static_cast<size_t>(errorTolerance_) > syncWord_.size()) {
        throw CorrelatorError("Error tolerance must be between 0 and the sync word size.");
    }

    CorrelationResult result;
    result.syncFound = false;
    result.payloadStartIndex = 0;
    
    if (hardBits.size() < syncWord_.size()) {
        return result; // Not an error, just not enough bits to find sync
    }

    // Sliding window correlation (Hamming distance)
    result.correlationMetrics.resize(hardBits.size() - syncWord_.size() + 1, 0.0f);
    
    int bestDistance = syncWord_.size() + 1;
    size_t bestIndex = 0;

    for (size_t i = 0; i <= hardBits.size() - syncWord_.size(); ++i) {
        int distance = 0;
        for (size_t j = 0; j < syncWord_.size(); ++j) {
            if (hardBits[i + j] != syncWord_[j]) {
                distance++;
            }
        }
        
        // normalized metric (0.0 = perfect match, 1.0 = completely different)
        result.correlationMetrics[i] = static_cast<float>(distance) / syncWord_.size();

        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = i;
        }
    }

    if (bestDistance <= errorTolerance_) {
        result.syncFound = true;
        result.payloadStartIndex = bestIndex + syncWord_.size();
        
        // Extract rest of bits as payload
        if (result.payloadStartIndex < hardBits.size()) {
            result.extractedPayload.assign(
                hardBits.begin() + result.payloadStartIndex,
                hardBits.end()
            );
        }
    }

    return result;
}

} // namespace module3
