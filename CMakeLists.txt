cmake_minimum_required(VERSION 3.22)
project(Locations VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Laufzeitbibliothek fest einbinden: das Plugin braucht dann kein VC++-Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(LOCATIONS_JUCE_PATH "" CACHE PATH "Lokaler JUCE-Ordner (leer = automatisch laden)")
set(LOCATIONS_FORMATS "VST3" CACHE STRING "Plugin-Formate")

if(LOCATIONS_JUCE_PATH)
    add_subdirectory(${LOCATIONS_JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.9
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Locations
    COMPANY_NAME "Stromschlag Audio"
    PLUGIN_MANUFACTURER_CODE Strm
    PLUGIN_CODE Loca
    FORMATS ${LOCATIONS_FORMATS}
    PRODUCT_NAME "Locations"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Fx Spatial
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Locations PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Locations PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(MSVC)
    target_compile_options(Locations PRIVATE /utf-8)
endif()

target_link_libraries(Locations
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
