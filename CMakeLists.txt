cmake_minimum_required(VERSION 3.22)
project(YanoSpace VERSION 0.1.0)

add_subdirectory(libs/JUCE)

if(APPLE)
    set(YANOSPACE_FORMATS VST3 AU Standalone)
else()
    set(YANOSPACE_FORMATS VST3 Standalone)
endif()

juce_add_plugin(YanoSpace
    COMPANY_NAME "Bumpin Audio"
    BUNDLE_ID "com.bumpinaudio.yanospace"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Nabs
    PLUGIN_CODE YnSp
    FORMATS ${YANOSPACE_FORMATS}
    PRODUCT_NAME "Yano Space")

target_sources(YanoSpace PRIVATE
    Source/YanoSpaceProcessor.cpp
    Source/PluginEditor.cpp
    Source/PluginEntry.cpp)

target_compile_definitions(YanoSpace PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(YanoSpace
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Dev-only headless audio render test (not shipped) -- see Source/AudioTestTool.cpp
juce_add_console_app(AudioTestTool PRODUCT_NAME "AudioTestTool")

target_sources(AudioTestTool PRIVATE
    Source/YanoSpaceProcessor.cpp
    Source/PluginEditor.cpp
    Source/AudioTestTool.cpp)

target_compile_definitions(AudioTestTool PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(AudioTestTool
    PRIVATE
        juce::juce_audio_formats
        juce::juce_audio_utils
        juce::juce_gui_basics
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags)

# Dev-only headless UI snapshot tool (not shipped) -- see Source/SnapshotTool.cpp
juce_add_console_app(SnapshotTool PRODUCT_NAME "SnapshotTool")

target_sources(SnapshotTool PRIVATE
    Source/YanoSpaceProcessor.cpp
    Source/PluginEditor.cpp
    Source/SnapshotTool.cpp)

target_compile_definitions(SnapshotTool PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(SnapshotTool
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_basics
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags)

# Dev-only listening aid, not part of the shipped repo -- renders a real
# WAV through the processor so the effect can be auditioned.
juce_add_console_app(RenderTool PRODUCT_NAME "RenderTool")

target_sources(RenderTool PRIVATE
    Source/YanoSpaceProcessor.cpp
    Source/PluginEditor.cpp
    Source/RenderTool.cpp)

target_compile_definitions(RenderTool PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(RenderTool
    PRIVATE
        juce::juce_audio_formats
        juce::juce_audio_utils
        juce::juce_gui_basics
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags)
