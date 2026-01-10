cmake_minimum_required(VERSION 3.20)
project(poor-house-juno VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Testing option (can be enabled without specifying platform)
option(BUILD_TESTS "Build unit tests" OFF)

# Platform detection
if(NOT DEFINED PLATFORM)
    if(EMSCRIPTEN)
        set(PLATFORM "web")
    elseif(BUILD_TESTS)
        set(PLATFORM "test")  # Test-only build, no platform-specific code
    else()
        set(PLATFORM "pi")
    endif()
endif()

message(STATUS "Building for platform: ${PLATFORM}")

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )

    # Optimization flags
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -ffast-math)
    endif()
endif()

# DSP library (shared between platforms)
add_library(phj_dsp STATIC
    src/dsp/oscillator.cpp
    src/dsp/dco.cpp
    src/dsp/filter.cpp
    src/dsp/envelope.cpp
    src/dsp/lfo.cpp
    src/dsp/voice.cpp
    src/dsp/chorus.cpp
    src/dsp/synth.cpp
)

target_include_directories(phj_dsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp
)

# Platform-specific builds
if(PLATFORM STREQUAL "web")
    message(STATUS "Configuring Web build (Emscripten)")

    # Web Audio worklet processor
    add_executable(synth-processor
        src/platform/web/main.cpp
    )

    target_link_libraries(synth-processor PRIVATE phj_dsp)

    # Emscripten-specific settings
    set_target_properties(synth-processor PROPERTIES
        SUFFIX ".js"
        LINK_FLAGS "\
            -O3 \
            -s WASM=1 \
            -s MODULARIZE=1 \
            -s EXPORT_NAME='createSynthProcessor' \
            -s EXPORTED_FUNCTIONS=['_malloc','_free'] \
            -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] \
            -s ALLOW_MEMORY_GROWTH=1 \
            -s INITIAL_MEMORY=16777216 \
            -s ENVIRONMENT='web,worker' \
            --no-entry \
            -lembind \
        "
    )

    # Copy output to web directory
    add_custom_command(TARGET synth-processor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_BINARY_DIR}/synth-processor.js
            ${CMAKE_CURRENT_SOURCE_DIR}/web/synth-processor.js
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_BINARY_DIR}/synth-processor.wasm
            ${CMAKE_CURRENT_SOURCE_DIR}/web/synth-processor.wasm
        COMMENT "Copying WASM files to web directory"
    )

elseif(PLATFORM STREQUAL "pi")
    message(STATUS "Configuring Raspberry Pi build (ALSA)")

    # Find ALSA
    find_package(ALSA REQUIRED)

    # Raspberry Pi executable
    add_executable(poor-house-juno
        src/platform/pi/main.cpp
        src/platform/pi/audio_driver.cpp
        src/platform/pi/midi_driver.cpp
    )

    target_link_libraries(poor-house-juno PRIVATE
        phj_dsp
        ALSA::ALSA
        pthread
    )

    target_include_directories(poor-house-juno PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/pi
    )

elseif(PLATFORM STREQUAL "test")
    message(STATUS "Configuring test-only build (no platform executable)")
    # No platform-specific executable, only DSP library and tests

else()
    message(FATAL_ERROR "Unknown platform: ${PLATFORM}")
endif()

# Testing
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
