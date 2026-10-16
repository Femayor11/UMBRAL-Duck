cmake_minimum_required(VERSION 3.22)

project(UmbralDuck VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Versiones fijadas (ver docs/DECISIONS.md)
set(UMBRAL_JUCE_TAG   "8.0.15" CACHE STRING "Tag exacto de JUCE")
set(UMBRAL_CATCH2_TAG "v3.9.1" CACHE STRING "Tag exacto de Catch2")

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        ${UMBRAL_JUCE_TAG}
    GIT_SHALLOW    ON)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        ${UMBRAL_CATCH2_TAG}
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE Catch2)

juce_add_plugin(UmbralDuck
    COMPANY_NAME "UMBRAL"
    PLUGIN_MANUFACTURER_CODE Umbr
    PLUGIN_CODE Udck
    FORMATS VST3 Standalone
    PRODUCT_NAME "UMBRAL Duck"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(UmbralDuck PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP/CurveBank.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP/DuckingEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP/GainSmoother.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP/PhaseClock.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP/TransportClock.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/Parameters/ParameterLayout.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/ControlStrip.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/GainResponseView.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/LevelMetersComponent.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/UmbralLookAndFeel.cpp")

target_include_directories(UmbralDuck PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Source")

target_compile_definitions(UmbralDuck PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(UmbralDuck
    PUBLIC
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(MSVC)
    target_compile_options(UmbralDuck PRIVATE /W4 /WX)
else()
    target_compile_options(UmbralDuck PRIVATE -Wall -Wextra -Werror)
endif()

# Pruebas
enable_testing()
add_executable(UmbralDuckTests
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/SmokeTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/PassThroughTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/DSPTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/GainAndParametersTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/PhaseClockTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/TransportClockTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/GainSmootherTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/UiStateSnapshotTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/LookAndFeelTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/GainResponseViewTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/WindowViewPersistenceTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/ControlStripTests.cpp")
target_compile_definitions(UmbralDuckTests PRIVATE JUCE_STANDALONE_APPLICATION=0)
target_link_libraries(UmbralDuckTests
    PRIVATE
        UmbralDuck
        Catch2::Catch2WithMain)
if(MSVC)
    target_compile_options(UmbralDuckTests PRIVATE /W4 /WX)
else()
    target_compile_options(UmbralDuckTests PRIVATE -Wall -Wextra -Werror)
endif()

include("${Catch2_SOURCE_DIR}/extras/Catch.cmake")
catch_discover_tests(UmbralDuckTests)
