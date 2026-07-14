cmake_minimum_required(VERSION 3.22)

project(VocalOne VERSION 1.0.0 LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# C++ standard
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Locate JUCE. Expected as a sibling directory: ../JUCE
# Can be overridden with -DJUCE_DIR=/path/to/JUCE
# ---------------------------------------------------------------------------
if(NOT DEFINED JUCE_DIR)
    set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" CACHE PATH "Path to the JUCE framework")
endif()

if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at ${JUCE_DIR}. Clone it there or pass -DJUCE_DIR=/path/to/JUCE.")
endif()

add_subdirectory(${JUCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/JUCE)

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------
juce_add_plugin(VocalOne
    COMPANY_NAME              "VocalOne Audio"
    BUNDLE_ID                 "com.vocalone.vocalone"
    PLUGIN_MANUFACTURER_CODE  Voco
    PLUGIN_CODE               Vco1
    FORMATS                   VST3 AU Standalone
    PRODUCT_NAME              "VocalOne"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    VST3_CATEGORIES           "Fx" "EQ" "Dynamics" "Pitch Shift"
    AU_MAIN_TYPE              "kAudioUnitType_Effect")

# ---------------------------------------------------------------------------
# Generate the JuceHeader.h and set config macros
# ---------------------------------------------------------------------------
juce_generate_juce_header(VocalOne)

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
target_sources(VocalOne PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h

    # DSP modules
    Source/DSP/AIAnalyzer.cpp
    Source/DSP/AIAnalyzer.h
    Source/DSP/SmartTune.cpp
    Source/DSP/SmartTune.h
    Source/DSP/AIEqualizer.cpp
    Source/DSP/AIEqualizer.h
    Source/DSP/AICompressor.cpp
    Source/DSP/AICompressor.h
    Source/DSP/AIDeEsser.cpp
    Source/DSP/AIDeEsser.h
    Source/DSP/AIGate.cpp
    Source/DSP/AIGate.h
    Source/DSP/NoiseRemoval.cpp
    Source/DSP/NoiseRemoval.h
    Source/DSP/Saturation.cpp
    Source/DSP/Saturation.h
    Source/DSP/HarmonicExciter.cpp
    Source/DSP/HarmonicExciter.h
    Source/DSP/FreshAir.cpp
    Source/DSP/FreshAir.h
    Source/DSP/StereoWidth.cpp
    Source/DSP/StereoWidth.h
    Source/DSP/Delay.cpp
    Source/DSP/Delay.h
    Source/DSP/Reverb.cpp
    Source/DSP/Reverb.h
    Source/DSP/Doubler.cpp
    Source/DSP/Doubler.h
    Source/DSP/Chorus.cpp
    Source/DSP/Chorus.h
    Source/DSP/FinalLimiter.cpp
    Source/DSP/FinalLimiter.h

    # AI modules
    Source/AI/AIAssistant.cpp
    Source/AI/AIAssistant.h
    Source/AI/SmartPreset.cpp
    Source/AI/SmartPreset.h

    # UI modules
    Source/UI/MainPanel.cpp
    Source/UI/MainPanel.h
    Source/UI/AnalyzerPanel.cpp
    Source/UI/AnalyzerPanel.h
    Source/UI/SmartTunePanel.cpp
    Source/UI/SmartTunePanel.h
    Source/UI/NeonLookAndFeel.cpp
    Source/UI/NeonLookAndFeel.h
    Source/UI/LUFSMeter.cpp
    Source/UI/LUFSMeter.h
    Source/UI/WaveformDisplay.cpp
    Source/UI/WaveformDisplay.h
    Source/UI/PitchGraph.cpp
    Source/UI/PitchGraph.h
    Source/UI/StereoScope.cpp
    Source/UI/StereoScope.h)

# ---------------------------------------------------------------------------
# Compile definitions (JUCE feature configuration)
# ---------------------------------------------------------------------------
target_compile_definitions(VocalOne PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_include_directories(VocalOne PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source)

# ---------------------------------------------------------------------------
# Link JUCE modules
# ---------------------------------------------------------------------------
target_link_libraries(VocalOne PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_plugin_client
    juce::juce_dsp
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
