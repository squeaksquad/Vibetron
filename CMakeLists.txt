cmake_minimum_required(VERSION 3.22)
project(Vibetron VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

set(JUCE_DIR "$ENV{HOME}/Dev/JUCE" CACHE PATH "Path to a JUCE 8 checkout")
add_subdirectory(${JUCE_DIR} JUCE)

# AAX: built when the Avid SDK is present. Unsigned AAX builds only load in Pro Tools Developer,
# so installing into Pro Tools' plug-in folder is opt-in (-DVIBETRON_INSTALL_AAX=ON).
set(AAX_SDK_DIR "$ENV{HOME}/Development/AAX/aax-sdk-2-9-0" CACHE PATH "Path to the Avid AAX SDK")
option(VIBETRON_INSTALL_AAX "Copy the AAX build into /Library/Application Support/Avid/Audio/Plug-Ins" OFF)

set(VIBETRON_FORMATS AU VST3 Standalone)
if(EXISTS "${AAX_SDK_DIR}/Interfaces/AAX.h")
    juce_set_aax_sdk_path("${AAX_SDK_DIR}")
    list(APPEND VIBETRON_FORMATS AAX)
    message(STATUS "Vibetron: AAX enabled (${AAX_SDK_DIR})")
else()
    message(STATUS "Vibetron: AAX SDK not found at ${AAX_SDK_DIR}; skipping AAX")
endif()

if(VIBETRON_INSTALL_AAX)
    set(VIBETRON_AAX_COPY_DIR "/Library/Application Support/Avid/Audio/Plug-Ins")
else()
    set(VIBETRON_AAX_COPY_DIR "${CMAKE_BINARY_DIR}/aax-staging")
endif()

juce_add_plugin(Vibetron
    COMPANY_NAME "Vibetron"
    BUNDLE_ID "com.vibetron.vt3"
    PLUGIN_MANUFACTURER_CODE Vbtr
    PLUGIN_CODE Vt3a
    FORMATS ${VIBETRON_FORMATS}
    PRODUCT_NAME "Vibetron VT-369"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx
    AAX_CATEGORY AAX_ePlugInCategory_Harmonic
    AAX_COPY_DIR "${VIBETRON_AAX_COPY_DIR}"
    COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(Vibetron PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/VUMeter.cpp
    Source/ModeSelector.cpp
    Source/Textures.cpp
    Source/TruePeakLimiter.cpp
    Source/CalPanel.cpp
    Source/EditHistory.cpp
    Source/UpdateChecker.cpp
    Source/TopBar.cpp)

target_compile_definitions(Vibetron PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Vibetron
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
