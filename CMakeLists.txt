cmake_minimum_required(VERSION 3.22)

project(PitchShifter VERSION 1.0.3 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

# JUCE: use a local checkout if JUCE_DIR is given, otherwise download it.
set(JUCE_DIR "" CACHE PATH "Path to a JUCE checkout (optional)")
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.10
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

set(PS_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PS_FORMATS AU)
endif()

juce_add_plugin(PitchShifter
    PRODUCT_NAME "PitchShifter"
    COMPANY_NAME "Ticco Ross"
    BUNDLE_ID "com.ticcoross.pitchshifter"
    PLUGIN_MANUFACTURER_CODE "Tico"
    PLUGIN_CODE "Tpsh"
    DESCRIPTION "PitchShifter"
    FORMATS ${PS_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Fx Pitch Shift
    AU_MAIN_TYPE kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(PitchShifter)

# Factory presets are compiled into the plugin.
file(GLOB PS_FACTORY_PRESETS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Common Files/Presets/*.xml")
juce_add_binary_data(PitchShifterPresets SOURCES ${PS_FACTORY_PRESETS})
set_target_properties(PitchShifterPresets PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_sources(PitchShifter PRIVATE
    Source/Source/PluginProcessor.cpp
    Source/Source/PitchEditor.cpp)

target_include_directories(PitchShifter PRIVATE Source/Source)

target_compile_definitions(PitchShifter PUBLIC
    JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

# Allow local builds on machines without ALSA/JACK dev headers.
option(PS_LINUX_AUDIO "Enable ALSA/JACK in the Linux standalone" ON)
if(UNIX AND NOT APPLE AND NOT PS_LINUX_AUDIO)
    target_compile_definitions(PitchShifter PUBLIC JUCE_ALSA=0 JUCE_JACK=0)
endif()

target_link_libraries(PitchShifter
    PRIVATE
        PitchShifterPresets
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Test host used by CI: checks that audio actually passes through the built plugin.
option(PS_BUILD_TESTS "Build the audio pass-through test host" OFF)
if(PS_BUILD_TESTS)
    juce_add_console_app(AudioPassTest PRODUCT_NAME "AudioPassTest" VERSION ${PROJECT_VERSION})
    target_sources(AudioPassTest PRIVATE tests/AudioPassTest.cpp)
    target_compile_definitions(AudioPassTest PRIVATE
        JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_AU=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(AudioPassTest PRIVATE
        juce::juce_audio_processors juce::juce_gui_basics juce::juce_recommended_config_flags)
endif()
