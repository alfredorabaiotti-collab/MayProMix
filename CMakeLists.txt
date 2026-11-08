cmake_minimum_required(VERSION 3.22)
# Windows: runtime statico, cosi' il plug-in non richiede il Visual C++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
cmake_policy(SET CMP0091 NEW)
project(MayProMix VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

# JUCE: usa una copia locale (-DJUCE_DIR=...) oppure la scarica automaticamente
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(MAY_FORMATS VST3)
if(APPLE)
    list(APPEND MAY_FORMATS AU)
endif()
option(MAY_BUILD_STANDALONE "Costruisce anche le app Standalone (per test)" OFF)
if(MAY_BUILD_STANDALONE)
    list(APPEND MAY_FORMATS Standalone)
endif()

juce_add_binary_data(MayProMixData SOURCES Resources/logo.png
    Resources/presets/proeq_template.json Resources/presets/proeq_metainfo.xml
    Resources/presets/comp_template.json Resources/presets/comp_metainfo.xml)

set(MAY_SHARED_SOURCES
    Source/Shared/SharedBus.cpp
    Source/Shared/Analyzer.cpp
    Source/Shared/PluginCatalog.cpp
    Source/Shared/Advisor.cpp)

function(may_configure target)
    target_compile_definitions(${target} PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)
    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8)
    endif()
    target_link_libraries(${target}
        PRIVATE
            MayProMixData
            juce::juce_audio_utils
            juce::juce_audio_processors
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endfunction()

# Plug-in da inserire su ogni traccia
juce_add_plugin(MayProMixTrack
    COMPANY_NAME "May Studio Recording"
    COMPANY_WEBSITE "https://vincemay.it"
    PRODUCT_NAME "May Pro Mix Track"
    BUNDLE_ID com.maystudio.promix.track
    PLUGIN_MANUFACTURER_CODE MaSt
    PLUGIN_CODE MpmT
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Analyzer
    AU_MAIN_TYPE kAudioUnitType_Effect
    FORMATS ${MAY_FORMATS})
target_sources(MayProMixTrack PRIVATE ${MAY_SHARED_SOURCES}
    Source/Track/TrackProcessor.cpp
    Source/Track/TrackEditor.cpp)
may_configure(MayProMixTrack)

# Plug-in da inserire sul canale Main
juce_add_plugin(MayProMixMaster
    COMPANY_NAME "May Studio Recording"
    COMPANY_WEBSITE "https://vincemay.it"
    PRODUCT_NAME "May Pro Mix Master"
    BUNDLE_ID com.maystudio.promix.master
    PLUGIN_MANUFACTURER_CODE MaSt
    PLUGIN_CODE MpmM
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Analyzer Mastering
    AU_MAIN_TYPE kAudioUnitType_Effect
    FORMATS ${MAY_FORMATS})
target_sources(MayProMixMaster PRIVATE ${MAY_SHARED_SOURCES}
    Source/Master/MasterProcessor.cpp
    Source/Master/MasterEditor.cpp)
may_configure(MayProMixMaster)
