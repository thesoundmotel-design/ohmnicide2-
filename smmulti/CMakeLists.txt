cmake_minimum_required(VERSION 3.22)

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
endif()

project(SoundMotelMultidrive VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.4
  GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

set(SM_FORMATS VST3 Standalone)
if(APPLE)
  list(APPEND SM_FORMATS AU)
endif()

juce_add_plugin(SoundMotelMultidrive
  COMPANY_NAME "The Sound Motel"
  BUNDLE_ID com.thesoundmotel.multidrive
  PLUGIN_MANUFACTURER_CODE SMtl
  PLUGIN_CODE Smdr
  FORMATS ${SM_FORMATS}
  PRODUCT_NAME "Sound Motel Multidrive"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES Fx Distortion
  AU_MAIN_TYPE kAudioUnitType_Effect)

juce_add_binary_data(SMMultiAssets SOURCES Assets/logo.png)

target_sources(SoundMotelMultidrive PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp)

target_compile_definitions(SoundMotelMultidrive PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SoundMotelMultidrive
  PRIVATE
    SMMultiAssets
    juce::juce_audio_utils
    juce::juce_audio_processors
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
