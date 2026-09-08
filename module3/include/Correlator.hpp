#pragma once

#include <vector>
#include <cstdint>

namespace module3 {

struct CorrelationResult {
    bool syncFound;
    size_t payloadStartIndex;
    std::vector<float> correlationMetrics;
    std::vector<uint8_t> extractedPayload;
};

class Correlator {
public:
    virtual ~Correlator() = default;

    /**
     * @brief Correlates the hard bits against a known sync word to find the payload.
     * @param hardBits The decoded hard bits from the FEC decoder.
     * @return CorrelationResult containing sync status and extracted payload.
     */
    virtual CorrelationResult correlateAndExtract(const std::vector<uint8_t>& hardBits) = 0;
};

class SyncWordCorrelator : public Correlator {
public:
    SyncWordCorrelator(const std::vector<uint8_t>& syncWord, int errorTolerance) 
        : syncWord_(syncWord), errorTolerance_(errorTolerance) {}

    CorrelationResult correlateAndExtract(const std::vector<uint8_t>& hardBits) override;

private:
    std::vector<uint8_t> syncWord_;
    int errorTolerance_;
};

} // namespace module3
