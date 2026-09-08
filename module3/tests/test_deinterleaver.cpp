#include "TestFramework.hpp"
#include "Deinterleaver.hpp"
#include "Module3Exceptions.hpp"
#include "../../module2/tests/RealDataLoader.hpp"
#include "../../module2/include/core/AnalysisOrchestrator.hpp"

using namespace module3::test;
using namespace module3;
using namespace module2::core;
using namespace module2::test;

std::shared_ptr<BitStream> getRealBitstream() {
    auto data = RealDataLoader::loadWAV("real_subset.wav");
    auto result = AnalysisOrchestrator::analyze(data);
    return result.bitStream;
}

void testBlockDeinterleaverRealData() {
    auto bitstream = getRealBitstream();
    check(bitstream != nullptr, "Real bitstream should not be null");

    // We don't know the exact length, so let's just pick dimensions that fit perfectly,
    // or truncate a copy of it to a known size.
    size_t size = bitstream->size();
    if (size == 0) {
        check(false, "Demodulated bitstream is empty");
        return;
    }
    
    // Choose dimensions: rows=2, cols=size/2
    size_t rows = 2;
    size_t cols = size / 2;
    
    // Create a truncated copy if size is odd
    std::shared_ptr<BitStream> testStream;
    if (bitstream->getType() == BitStreamType::SOFT_FLOAT) {
        auto softIn = std::static_pointer_cast<SoftBitStreamFloat>(bitstream);
        auto truncated = std::make_shared<SoftBitStreamFloat>();
        truncated->llrs.assign(softIn->llrs.begin(), softIn->llrs.begin() + (rows * cols));
        testStream = truncated;
    } else {
        auto hardIn = std::static_pointer_cast<HardBitStream>(bitstream);
        auto truncated = std::make_shared<HardBitStream>();
        truncated->bits.assign(hardIn->bits.begin(), hardIn->bits.begin() + (rows * cols));
        testStream = truncated;
    }

    BlockDeinterleaver deinterleaver(rows, cols);
    auto out = deinterleaver.deinterleave(testStream);
    
    check(out != nullptr, "Deinterleaver output should not be null for real data");
    check(out->size() == rows * cols, "Output size mismatch on real data");
}

void testBlockDeinterleaverExceptions() {
    BlockDeinterleaver deinterleaver(2, 3);
    
    // Test Nullptr
    bool threwNull = false;
    try {
        deinterleaver.deinterleave(nullptr);
    } catch (const DeinterleaverError& e) {
        threwNull = true;
    }
    check(threwNull, "Deinterleaver should throw DeinterleaverError on null input");
    
    // Test Dimension Mismatch
    auto badStream = std::make_shared<HardBitStream>();
    badStream->bits = {1, 0, 1}; // size 3, expected 6
    bool threwDim = false;
    try {
        deinterleaver.deinterleave(badStream);
    } catch (const DeinterleaverError& e) {
        threwDim = true;
    }
    check(threwDim, "Deinterleaver should throw DeinterleaverError on dimension mismatch");
}

void run_deinterleaver_tests() {
    TEST_CASE("BlockDeinterleaver - Real Data Processing", testBlockDeinterleaverRealData);
    TEST_CASE("BlockDeinterleaver - Custom Exception Handling", testBlockDeinterleaverExceptions);
}
