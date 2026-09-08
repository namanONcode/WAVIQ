#include "TestFramework.hpp"
#include "Correlator.hpp"
#include "Module3Exceptions.hpp"
#include "../../module2/tests/RealDataLoader.hpp"
#include "../../module2/include/core/AnalysisOrchestrator.hpp"
#include "FecDecoder.hpp"

using namespace module3::test;
using namespace module3;
using namespace module2::core;
using namespace module2::test;

// Forward decl of helper
std::shared_ptr<BitStream> getRealBitstream();

void testCorrelatorRealData() {
    auto bitstream = getRealBitstream();
    
    // Pass it through our Viterbi skeleton to get hard bits
    ViterbiDecoder viterbi(3, {7, 5});
    auto fecResult = viterbi.decode(bitstream);
    
    auto hardBits = fecResult.decodedBits->bits;
    check(hardBits.size() > 0, "Hard bits should not be empty");
    
    // We'll extract a portion of the hard bits to act as our sync word so it's guaranteed to be found
    std::vector<uint8_t> syncWord;
    if (hardBits.size() >= 10) {
        syncWord.assign(hardBits.begin() + 2, hardBits.begin() + 7); // arbitrary subset
    } else {
        syncWord = {0, 1};
    }
    
    SyncWordCorrelator correlator(syncWord, 0); 
    auto result = correlator.correlateAndExtract(hardBits);
    
    check(result.syncFound, "Correlator should find the extracted sync word from real data");
}

void testCorrelatorExceptions() {
    // Empty sync word
    bool threwEmpty = false;
    try {
        SyncWordCorrelator correlator({}, 0);
        correlator.correlateAndExtract({1, 0, 1});
    } catch (const CorrelatorError& e) {
        threwEmpty = true;
    }
    check(threwEmpty, "Correlator should throw CorrelatorError on empty sync word");

    // Invalid error tolerance
    bool threwTol = false;
    try {
        SyncWordCorrelator correlator({1, 1}, 5); // tolerance > size
        correlator.correlateAndExtract({1, 0, 1});
    } catch (const CorrelatorError& e) {
        threwTol = true;
    }
    check(threwTol, "Correlator should throw CorrelatorError on invalid error tolerance");
}

void run_correlator_tests() {
    TEST_CASE("SyncWordCorrelator - Real Data Processing", testCorrelatorRealData);
    TEST_CASE("SyncWordCorrelator - Custom Exception Handling", testCorrelatorExceptions);
}
