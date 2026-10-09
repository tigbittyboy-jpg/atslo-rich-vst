cmake_minimum_required(VERSION 3.22)
project(ATSLO_RICH_SYNTH VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "macOS architectures")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

set(ATSLO_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/HybridSynth.cpp
    Source/DSP/Effects.cpp
    Source/Presets/PresetManager.cpp)

juce_add_plugin(ATSLO_Rich_Synth
    COMPANY_NAME ATSLO
    BUNDLE_ID com.atslo.richsynth
    PLUGIN_MANUFACTURER_CODE Atsl
    PLUGIN_CODE Rich
    FORMATS VST3 Standalone
    PRODUCT_NAME "ATSLO RICH SYNTH"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(ATSLO_Rich_Synth)
target_sources(ATSLO_Rich_Synth PRIVATE ${ATSLO_SOURCES})
target_include_directories(ATSLO_Rich_Synth PRIVATE Source)
target_compile_definitions(ATSLO_Rich_Synth PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(ATSLO_Rich_Synth PRIVATE
    juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

option(ATSLO_BUILD_TESTS "Build ATSLO tests" ON)
if(ATSLO_BUILD_TESTS)
    enable_testing()
    juce_add_console_app(ATSLO_Tests PRODUCT_NAME "ATSLO Tests")
    juce_generate_juce_header(ATSLO_Tests)
    target_sources(ATSLO_Tests PRIVATE
        Tests/TestMain.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp
        Source/DSP/HybridSynth.cpp Source/DSP/Effects.cpp
        Source/Presets/PresetManager.cpp)
    target_include_directories(ATSLO_Tests PRIVATE Source)
    target_compile_definitions(ATSLO_Tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(ATSLO_Tests PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_test(NAME atslo_tests COMMAND ATSLO_Tests)
endif()
