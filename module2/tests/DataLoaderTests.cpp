#include "TestFramework.hpp"
#include "RealDataLoader.hpp"
#include "core/Module2Exceptions.hpp"
#include <fstream>
#include <stdexcept>
#include <cstdio>

using namespace module2::test;
using namespace module2;

class DataLoaderTests {
public:
    static void runTests() {
        std::cout << "[DataLoader] Running tests...\n";
        testInvalidHeader();
        testTruncatedIQ();
        testMissingFmtChunk();
    }

private:
    static void testInvalidHeader() {
        bool threw = false;
        try {
            auto data = RealDataLoader::loadWAV("hdf5_fixture_malformed_frame_len_mismatch.h5");
        } catch (const DataLoaderError& e) {
            threw = true;
            std::string msg = e.what();
            check(msg.find("libsndfile error") != std::string::npos, 
                  "Exception message should mention libsndfile error for invalid format");
        }
        check(threw, "Should throw on invalid header");
    }

    static void testTruncatedIQ() {
        std::string tmpFile = RealDataLoader::getTestDataDir() + "test_truncated.iq";
        std::ofstream out(tmpFile, std::ios::binary);
        out.write("12345", 5); 
        out.close();

        bool threw = false;
        try {
            auto data = RealDataLoader::loadIQFloat32("test_truncated.iq");
        } catch (const DataLoaderError& e) {
            threw = true;
            std::string msg = e.what();
            check(msg.find("not a multiple of complex float size") != std::string::npos, 
                  "Exception message should mention not a multiple of complex float size");
        }
        
        std::remove(tmpFile.c_str());
        check(threw, "Should throw on truncated IQ");
    }

    static void testMissingFmtChunk() {
        std::string tmpFile = RealDataLoader::getTestDataDir() + "test_missing_fmt.wav";
        std::ofstream out(tmpFile, std::ios::binary);
        out.write("RIFF\x24\x00\x00\x00WAVE", 12);
        out.write("data\x00\x00\x00\x00", 8); 
        out.close();

        bool threw = false;
        try {
            auto data = RealDataLoader::loadWAV("test_missing_fmt.wav");
        } catch (const DataLoaderError& e) {
            threw = true;
            std::string msg = e.what();
            check(msg.find("libsndfile error") != std::string::npos, 
                  "Exception message should mention libsndfile error for missing fmt chunk");
        }

        std::remove(tmpFile.c_str());
        check(threw, "Should throw on missing fmt chunk");
    }
};

void runDataLoaderTests() {
    DataLoaderTests::runTests();
}
