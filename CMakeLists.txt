cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
project(8ceeVintage VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ON for build machines without a desktop session (GitLab's hosted Windows runners): patches JUCE's juceaide
# helper so it doesn't start the GUI system, and skips the VST3 moduleinfo step (it loads the plug-in).
option(ECEE_HEADLESS_CI "Build on a machine without a desktop session" OFF)

# Use a local JUCE checkout with -DJUCE_PATH=/path/to/JUCE, otherwise it is downloaded.
set(JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout (optional)")
if(JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    if(ECEE_HEADLESS_CI)
        set(ecee_juce_patch PATCH_COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch-juceaide.cmake")
    endif()
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON
        ${ecee_juce_patch})
    FetchContent_MakeAvailable(JUCE)
endif()

set(ECEE_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND ECEE_FORMATS AU)
endif()

# Set ON to copy the built plug-ins into the system plug-in folders.
option(ECEE_COPY_PLUGINS "Copy plug-ins to the system plug-in folders after building" OFF)

if(ECEE_HEADLESS_CI)
    set(ecee_vst3_manifest FALSE)
else()
    set(ecee_vst3_manifest TRUE)
endif()

function(ecee_add_plugin target product code category)
    juce_add_plugin(${target}
        COMPANY_NAME "8cee"
        COMPANY_WEBSITE ""
        BUNDLE_ID "com.eightcee.${target}"
        PLUGIN_MANUFACTURER_CODE Ecee
        PLUGIN_CODE ${code}
        PRODUCT_NAME "${product}"
        FORMATS ${ECEE_FORMATS}
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD ${ECEE_COPY_PLUGINS}
        VST3_AUTO_MANIFEST ${ecee_vst3_manifest}
        VST3_CATEGORIES Fx ${category}
        AU_MAIN_TYPE kAudioUnitType_Effect)

    target_sources(${target} PRIVATE
        plugins/${target}/PluginProcessor.cpp
        plugins/${target}/PluginEditor.cpp)

    target_include_directories(${target} PRIVATE shared plugins/${target})

    target_compile_definitions(${target} PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    target_link_libraries(${target}
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endfunction()

ecee_add_plugin(FET8  "8cee FET-8"  Fet8 Dynamics)
ecee_add_plugin(Bus8  "8cee Bus-8"  Bus8 Dynamics)
ecee_add_plugin(Reel8 "8cee Reel-8" Rel8 Distortion)

# Dev tools (UI snapshots + offline DSP checks). Not needed to build the plug-ins.
option(ECEE_BUILD_TOOLS "Build the snapshot/test tools" OFF)
if(ECEE_BUILD_TOOLS)
    add_subdirectory(tools)
endif()
