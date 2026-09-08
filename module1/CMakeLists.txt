cmake_minimum_required(VERSION 3.16)
project(module1_signal_io LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED sndfile)
# CMake's bundled FindHDF5 module (not an HDF5-exported config package) --
# chosen because it works across older libhdf5 installs that don't ship a
# modern hdf5-config.cmake (this project's build environment: 1.10.10).
find_package(HDF5 REQUIRED COMPONENTS C)
find_package(Threads REQUIRED)

add_library(module1_core
    src/WavLoader.cpp
    src/IqLoader.cpp
    src/IqSampleDecoder.cpp
    src/IqMetadataParser.cpp
    src/LoaderFactory.cpp
    src/HdfIqFrameDataset.cpp
    src/SignalSessionModel.cpp
    src/VisualizationProcessors.cpp
    src/VisualizationAnalysisExecutor.cpp
)
target_include_directories(module1_core PUBLIC 
    include 
    ${SNDFILE_INCLUDE_DIRS} 
    ${HDF5_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../module2/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../module3/include
)
target_link_libraries(module1_core PUBLIC 
    ${SNDFILE_LIBRARIES} 
    ${HDF5_C_LIBRARIES} 
    Threads::Threads
    ${CMAKE_CURRENT_SOURCE_DIR}/../module3/build/libBitstreamProcessorEngine.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../module2/build/libSignalAnalyzerEngine.a
    fftw3 fftw3f fec
)
target_compile_options(module1_core PUBLIC ${SNDFILE_CFLAGS_OTHER})

add_executable(module1_cli src/main.cpp)
target_link_libraries(module1_cli PRIVATE module1_core)

# Conditional GUI build
find_package(Qt6 COMPONENTS Widgets OPTIONAL_COMPONENTS Test Gui)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets OPTIONAL_COMPONENTS Test Gui)
endif()

if(Qt6_FOUND OR Qt5_FOUND)
    if(Qt6_FOUND)
        set(QT_CORE_TARGET Qt6::Core)
        set(QT_WIDGETS_TARGET Qt6::Widgets)
    else()
        set(QT_CORE_TARGET Qt5::Core)
        set(QT_WIDGETS_TARGET Qt5::Widgets)
    endif()

    add_executable(module1_gui 
        src/gui_main.cpp 
        src/QtVisualizationView.cpp
        include/module1/QtVisualizationView.h
    )
    target_link_libraries(module1_gui PRIVATE module1_core ${QT_WIDGETS_TARGET})
endif()

enable_testing()
add_subdirectory(tests)
