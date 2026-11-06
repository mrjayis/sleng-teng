cmake_minimum_required(VERSION 3.22)
project(Teng40 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
endif()

option(TENG_BUILD_PLUGIN "Build the JUCE plugin (VST3/AU/Standalone)" ON)
option(TENG_BUILD_TOOLS  "Build the offline render / smoke-test tool" ON)

# --- framework-free DSP core ------------------------------------------------
add_library(teng_dsp STATIC dsp/Engine.cpp dsp/Upd931.cpp)
target_include_directories(teng_dsp PUBLIC dsp)

if(TENG_BUILD_TOOLS)
    add_executable(teng_render tools/render.cpp)
    target_link_libraries(teng_render PRIVATE teng_dsp)
    enable_testing()
    add_test(NAME render_smoke COMMAND teng_render ${CMAKE_BINARY_DIR}/teng40-demo.wav)
endif()

# --- plugin -----------------------------------------------------------------
if(TENG_BUILD_PLUGIN)
    include(FetchContent)
    # Point FETCHCONTENT_SOURCE_DIR_JUCE at a local checkout to skip the download.
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)

    set(TENG_FORMATS VST3 Standalone)
    if(APPLE)
        list(APPEND TENG_FORMATS AU)
    endif()

    juce_add_plugin(Teng40
        COMPANY_NAME "Teng Audio"
        BUNDLE_ID "com.tengaudio.teng40"
        PLUGIN_MANUFACTURER_CODE Tnga
        PLUGIN_CODE T40k
        FORMATS ${TENG_FORMATS}
        PRODUCT_NAME "Teng-40"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Instrument Synth
        AU_MAIN_TYPE kAudioUnitType_MusicDevice)

    target_sources(Teng40 PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp)

    target_compile_definitions(Teng40 PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    target_link_libraries(Teng40
        PRIVATE
            teng_dsp
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endif()
