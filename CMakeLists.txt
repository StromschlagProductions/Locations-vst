cmake_minimum_required(VERSION 3.22)
# Versionsnummer steht in der Datei VERSION (wird bei jeder Aenderung hochgezaehlt)
file(STRINGS VERSION LOC_VERSION LIMIT_COUNT 1)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS VERSION)
project(Locations VERSION ${LOC_VERSION})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Laufzeitbibliothek fest einbinden: das Plugin braucht dann kein VC++-Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(AUDIO_DSP_ONLY "Build portable audio regression tests without JUCE" OFF)
include(CTest)
if(BUILD_TESTING)
    add_executable(AudioDSPTests Tests/DSPTests.cpp)
    target_include_directories(AudioDSPTests PRIVATE Source)
    add_test(NAME AudioDSP COMMAND AudioDSPTests)
endif()
if(AUDIO_DSP_ONLY)
    return()
endif()

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

juce_add_binary_data(LocationsAssets SOURCES Assets/logo.png)

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
    PLUGIN_VERSION_TEXT="${LOC_VERSION}"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(MSVC)
    target_compile_options(Locations PRIVATE /utf-8)
endif()

target_link_libraries(Locations
    PRIVATE
        LocationsAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(BUILD_TESTING)
    juce_add_console_app(AudioProcessorTests PRODUCT_NAME "Audio processor regression tests")
    target_sources(AudioProcessorTests PRIVATE Tests/ProcessorTests.cpp)
    target_include_directories(AudioProcessorTests PRIVATE Source $<TARGET_PROPERTY:Locations,INCLUDE_DIRECTORIES>)
    target_compile_definitions(AudioProcessorTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(AudioProcessorTests PRIVATE Locations)
    add_test(NAME AudioProcessor COMMAND AudioProcessorTests)
endif()
