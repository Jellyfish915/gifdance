cmake_minimum_required(VERSION 3.22)

project(GifDance VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(vendor/JUCE)

juce_add_plugin(GifDance
    COMPANY_NAME "Codex"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Cdx1
    PLUGIN_CODE Gfdn
    FORMATS VST3
    PRODUCT_NAME "GifDance")

juce_generate_juce_header(GifDance)

target_sources(GifDance
    PRIVATE
        vendor/gifdec/gifdec.c
        src/GifDecoder.cpp
        src/PluginEditor.cpp
        src/PluginProcessor.cpp)

target_include_directories(GifDance
    PRIVATE
        src
        vendor/gifdec)

target_compile_definitions(GifDance
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
    PRIVATE
        _CRT_NONSTDC_NO_WARNINGS=1
        _CRT_SECURE_NO_WARNINGS=1)

target_compile_options(GifDance
    PRIVATE
        $<$<C_COMPILER_ID:MSVC>:/utf-8>
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(GifDance
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
