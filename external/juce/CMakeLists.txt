cmake_minimum_required(VERSION 2.8.12)
project(JUCE)

if(NOT ML_JUCE_DIR)
  set(ML_JUCE_DIR "${CMAKE_CURRENT_LIST_DIR}/JUCE")
endif()

target_include_directories(madronalib PUBLIC "${ML_JUCE_DIR}")

if(NOT ML_JUCE_HEADER_PATH)
  message(FATAL_ERROR "Madronalib requires ML_JUCE_HEADER_PATH to be set")
endif()

macro(add_juce_library LIB EXTENSION)
  add_library(${LIB}
    "${ML_JUCE_DIR}/modules/${LIB}/${LIB}.h"
    "${ML_JUCE_DIR}/modules/${LIB}/${LIB}.${EXTENSION}")
  target_include_directories(${LIB} PUBLIC ${ML_JUCE_HEADER_PATH})
  target_include_directories(${LIB} PUBLIC ${ML_JUCE_DIR})
endmacro()

add_juce_library(juce_audio_basics mm)
target_link_libraries(juce_audio_basics "-framework Accelerate")
target_link_libraries(juce_audio_basics juce_core)

add_juce_library(juce_audio_devices mm)
target_link_libraries(juce_audio_devices "-framework AudioToolbox")
target_link_libraries(juce_audio_devices "-framework CoreAudio")
target_link_libraries(juce_audio_devices "-framework CoreMIDI")
target_link_libraries(juce_audio_devices juce_events)
target_link_libraries(juce_audio_devices juce_audio_basics)
target_link_libraries(juce_audio_devices juce_audio_formats)

add_juce_library(juce_audio_formats mm)
target_link_libraries(juce_audio_formats juce_audio_basics)

add_juce_library(juce_audio_processors mm)
target_link_libraries(juce_audio_processors juce_gui_extra)
target_link_libraries(juce_audio_processors juce_audio_basics)

add_juce_library(juce_audio_utils mm)
target_link_libraries(juce_audio_utils juce_gui_basics)
target_link_libraries(juce_audio_utils juce_audio_devices)
target_link_libraries(juce_audio_utils juce_audio_processors)
target_link_libraries(juce_audio_utils juce_audio_formats)

add_juce_library(juce_box2d cpp)
target_link_libraries(juce_box2d juce_graphics)

add_juce_library(juce_core mm)

add_juce_library(juce_cryptography mm)
target_link_libraries(juce_cryptography juce_core)

add_juce_library(juce_data_structures mm)
target_link_libraries(juce_data_structures juce_core)
target_link_libraries(juce_data_structures juce_events)

add_juce_library(juce_events mm)
target_link_libraries(juce_events juce_core)

add_juce_library(juce_graphics mm)
target_link_libraries(juce_graphics "-framework CoreGraphics")
target_link_libraries(juce_graphics "-framework CoreText")
target_link_libraries(juce_graphics "-framework Cocoa")
target_link_libraries(juce_graphics "-framework QuartzCore")
target_link_libraries(juce_graphics juce_core)
target_link_libraries(juce_graphics juce_events)

add_juce_library(juce_gui_basics mm)
target_link_libraries(juce_gui_basics "-framework CoreGraphics")
target_link_libraries(juce_gui_basics juce_data_structures)
target_link_libraries(juce_gui_basics juce_events)
target_link_libraries(juce_gui_basics juce_graphics)
target_link_libraries(juce_gui_basics juce_core)

add_juce_library(juce_gui_extra mm)
target_link_libraries(juce_graphics "-framework WebKit")
target_link_libraries(juce_gui_extra juce_gui_basics)

add_juce_library(juce_opengl mm)
target_link_libraries(juce_opengl juce_gui_basics)
target_link_libraries(juce_opengl "-framework OpenGL")

add_juce_library(juce_tracktion_marketplace cpp)
target_link_libraries(juce_tracktion_marketplace juce_data_structures)
target_link_libraries(juce_tracktion_marketplace juce_cryptography)

add_juce_library(juce_video mm)
target_link_libraries(juce_video juce_gui_extra)
