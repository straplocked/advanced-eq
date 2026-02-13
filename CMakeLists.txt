cmake_minimum_required(VERSION 3.25)

# Deployment target must be set before project() to propagate to all sub-builds
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binary")
endif()

set(ADVANCED_EQ_VERSION_MAJOR 0)
set(ADVANCED_EQ_VERSION_MINOR 1)
set(ADVANCED_EQ_VERSION_PATCH 0)

project(AdvancedEQ
    VERSION ${ADVANCED_EQ_VERSION_MAJOR}.${ADVANCED_EQ_VERSION_MINOR}.${ADVANCED_EQ_VERSION_PATCH}
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Platform defaults
include(cmake/PlatformDefaults.cmake)

# JUCE
add_subdirectory(JUCE)

# CLAP support
set(CLAP_JUCE_EXTENSIONS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/clap-juce-extensions" CACHE STRING "")
if(EXISTS "${CLAP_JUCE_EXTENSIONS_DIR}/CMakeLists.txt")
    add_subdirectory(${CLAP_JUCE_EXTENSIONS_DIR} clap-juce-extensions)
    set(HAS_CLAP TRUE)
else()
    message(STATUS "clap-juce-extensions not found, CLAP format will not be built")
    set(HAS_CLAP FALSE)
endif()

# Plugin formats
set(FORMATS Standalone VST3)
if(APPLE)
    list(APPEND FORMATS AU)
endif()
if(DEFINED AAX_SDK_PATH)
    juce_set_aax_sdk_path(${AAX_SDK_PATH})
    list(APPEND FORMATS AAX)
endif()

# --- Plugin Target ---
juce_add_plugin(AdvancedEQ
    COMPANY_NAME "AdvancedEQ"
    BUNDLE_ID "com.advancedeq.advancedeq"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE AEqP
    PLUGIN_CODE AEq1
    FORMATS ${FORMATS}
    PRODUCT_NAME "Advanced EQ"
    VST3_CATEGORIES "Fx" "EQ" "Analyzer"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
    AAX_CATEGORY "AAX_ePlugInCategory_EQ"
)

# CLAP extension
if(HAS_CLAP)
    clap_juce_extensions_plugin(
        TARGET AdvancedEQ
        CLAP_ID "com.advancedeq.advancedeq"
        CLAP_FEATURES "audio-effect" "equalizer" "analyzer"
        CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES 64
        CLAP_USE_JUCE_PARAMETER_RANGES DISCRETE
    )
endif()

# --- Binary Data (fonts, icons) ---
juce_add_binary_data(AdvancedEQBinaryData SOURCES
    resources/fonts/SpaceGrotesk-Regular.ttf
    resources/fonts/Audiowide-Regular.ttf
)

# --- Source Files ---
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h"
)
target_sources(AdvancedEQ PRIVATE ${SourceFiles})
target_include_directories(AdvancedEQ PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/source")

# --- JUCE Module Dependencies ---
target_link_libraries(AdvancedEQ
    PRIVATE
        AdvancedEQBinaryData
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_opengl
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# --- Preprocessor Definitions ---
target_compile_definitions(AdvancedEQ
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=0
    PRIVATE
        ADVANCED_EQ_VERSION="${PROJECT_VERSION}"
)

# --- Fast math for DSP performance ---
if(NOT MSVC)
    target_compile_options(AdvancedEQ PRIVATE -ffast-math)
else()
    target_compile_options(AdvancedEQ PRIVATE /fp:fast)
endif()

# --- Testing ---
option(ADVANCED_EQ_BUILD_TESTS "Build tests" OFF)
if(ADVANCED_EQ_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
