add_executable(module1_tests test_module1.cpp)
target_link_libraries(module1_tests PRIVATE module1_core)
target_compile_definitions(module1_tests PRIVATE TESTDATA_DIR="${PROJECT_SOURCE_DIR}/testdata")
add_test(NAME module1_tests COMMAND module1_tests)
