cmake_minimum_required(VERSION 3.16)
project(module1_signal_io LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED sndfile)
find_package(nlohmann_json REQUIRED)
# CMake's bundled FindHDF5 module (not an HDF5-exported config package) --
# chosen because it works across older libhdf5 installs that don't ship a
# modern hdf5-config.cmake (this project's build environment: 1.10.10).
find_package(HDF5 REQUIRED COMPONENTS C)

add_library(module1_core
    src/WavLoader.cpp
    src/IqLoader.cpp
    src/IqSampleDecoder.cpp
    src/IqMetadataParser.cpp
    src/LoaderFactory.cpp
    src/HdfIqFrameDataset.cpp
)
target_include_directories(module1_core PUBLIC include ${SNDFILE_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
target_link_libraries(module1_core PUBLIC ${SNDFILE_LIBRARIES} nlohmann_json::nlohmann_json ${HDF5_C_LIBRARIES})
target_compile_options(module1_core PUBLIC ${SNDFILE_CFLAGS_OTHER})

add_executable(module1_cli src/main.cpp)
target_link_libraries(module1_cli PRIVATE module1_core)

enable_testing()
add_subdirectory(tests)
