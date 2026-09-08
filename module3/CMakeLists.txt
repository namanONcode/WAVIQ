cmake_minimum_required(VERSION 3.14)
project(Module3_BitstreamProcessing VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add include directory
include_directories(include)

# Module 2 dependency for AnalysisResult and BitStream
include_directories(../module2/include)

enable_testing()

# Module 3 Core Library
add_library(BitstreamProcessorEngine 
    src/Deinterleaver.cpp
    src/FecDecoder.cpp
    src/Correlator.cpp
)

target_include_directories(BitstreamProcessorEngine PUBLIC include)
target_include_directories(BitstreamProcessorEngine PRIVATE ../module2/include)
target_link_libraries(BitstreamProcessorEngine PUBLIC fec)

# We link to Module 2 to consume its data structures if needed
# (Though AnalysisResult is mostly header-only, we might need some symbols)
# target_link_libraries(BitstreamProcessorEngine PRIVATE SignalAnalyzerEngine)

# Test Executable
add_executable(BitstreamProcessorTests
    tests/test_deinterleaver.cpp
    tests/test_fec.cpp
    tests/test_correlator.cpp
    tests/main.cpp
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED sndfile)

find_library(FFTW3_LIB NAMES fftw3 fftw3-3 HINTS "D:/SIH2K26/WAVIQ/fftw3")
find_library(FFTW3F_LIB NAMES fftw3f fftw3f-3 HINTS "D:/SIH2K26/WAVIQ/fftw3")

target_include_directories(BitstreamProcessorTests PRIVATE include ../module2/include ${SNDFILE_INCLUDE_DIRS})
target_link_libraries(BitstreamProcessorTests PRIVATE 
    BitstreamProcessorEngine 
    ${CMAKE_CURRENT_SOURCE_DIR}/../module2/build/libSignalAnalyzerEngine.a 
    ${SNDFILE_LIBRARIES}
    ${FFTW3_LIB}
    ${FFTW3F_LIB}
    fec
)
add_test(NAME BitstreamProcessorTests COMMAND BitstreamProcessorTests)
