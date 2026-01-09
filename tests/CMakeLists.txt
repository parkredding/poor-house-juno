# Unit tests for Poor House Juno DSP components
cmake_minimum_required(VERSION 3.20)

# Download and configure Catch2 v3
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.1
)
FetchContent_MakeAvailable(Catch2)

# Test executable
add_executable(phj_tests
    test_main.cpp
    test_oscillator.cpp
    test_filter.cpp
    test_envelope.cpp
    test_lfo.cpp
    test_voice.cpp
)

target_link_libraries(phj_tests PRIVATE
    phj_dsp
    Catch2::Catch2WithMain
)

target_include_directories(phj_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/dsp
)

# Enable CTest integration
include(CTest)
include(Catch)
catch_discover_tests(phj_tests)

# Add custom test target for convenience
add_custom_target(run_tests
    COMMAND phj_tests
    DEPENDS phj_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running unit tests..."
)
