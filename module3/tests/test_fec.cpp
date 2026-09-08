#include "TestFramework.hpp"
#include "FecDecoder.hpp"
#include "Module3Exceptions.hpp"
#include "../../module2/tests/RealDataLoader.hpp"
#include "../../module2/include/core/AnalysisOrchestrator.hpp"

using namespace module3::test;
using namespace module3;
using namespace module2::core;
using namespace module2::test;

// Forward decl of helper from test_deinterleaver
std::shared_ptr<BitStream> getRealBitstream();

void testFecPipelinesRealData() {
    auto bitstream = getRealBitstream();
    check(bitstream != nullptr, "Real bitstream should not be null");
    
    // We expect the real data (WAV) to produce soft float bits from Module 2's QPSK/BPSK demod
    check(bitstream->getType() == BitStreamType::SOFT_FLOAT || bitstream->getType() == BitStreamType::HARD, 
          "Bitstream should be valid");

    auto viterbi = std::make_shared<ViterbiDecoder>(3, std::vector<int>{7, 5});
    auto rs = std::make_shared<RSDecoder>(8, 255, 223);
    
    ConcatenatedDecoder concatenated(viterbi, rs);
    
    auto result = concatenated.decode(bitstream);
    
    check(result.success, "Concatenated decoding on real data should report success");
    check(result.decodedBits != nullptr, "Decoded bits should not be null");
    check(result.decodedBits->size() > 0, "Decoded bits should not be empty");
}

void testFecExceptions() {
    ViterbiDecoder viterbi(3, {7, 5});
    
    // Null Input
    bool threwNull = false;
    try {
        viterbi.decode(nullptr);
    } catch (const FecDecoderError& e) {
        threwNull = true;
    }
    check(threwNull, "FEC Decoder should throw FecDecoderError on null input");
    
    // Concatenated Decoder Initialization Error
    bool threwInit = false;
    try {
        ConcatenatedDecoder badConcat(nullptr, nullptr);
        auto dummyIn = std::make_shared<HardBitStream>();
        badConcat.decode(dummyIn);
    } catch (const FecDecoderError& e) {
        threwInit = true;
    }
    check(threwInit, "Concatenated Decoder should throw FecDecoderError on null internal decoders");
}

void run_fec_tests() {
    TEST_CASE("FEC Pipeline - Real Data Processing", testFecPipelinesRealData);
    TEST_CASE("FEC Pipeline - Custom Exception Handling", testFecExceptions);
}
