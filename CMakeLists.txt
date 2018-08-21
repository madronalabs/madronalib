# madronalib/CMakeLists.txt
# root CMake file for madronalib project.

project(madronalib CXX)

cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(madronalib_VERSION_MAJOR "0")
set(madronalib_VERSION_MINOR "1")
set(madronalib_VERSION_PATCH "0")
set(madronalib_VERSION_EXTRA "")
set(madronalib_VERSION "${madronalib_VERSION_MAJOR}.${madronalib_VERSION_MINOR}")
set(madronalib_VERSION_FULL "${madronalib_VERSION}.${madronalib_VERSION_PATCH}${madronalib_VERSION_EXTRA}")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# with BUILD_NEW_ONLY on (the default), only new modules not dependent
# on dual-licensed JUCE code will be compiled. For use in the Soundplane
# app, which currently relies on JUCE for graphics, set BUILD_NEW_ONLY to OFF.
# Either way madronalib may be built independently.

option(BUILD_NEW_ONLY "Build only the modernized codebase" ON)

option(ML_BUILD_EXAMPLES "Build the examples" ON)
option(ML_BUILD_TESTS "Build the ML test programs" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ML_BUILD_DOCS "Build the ML documentation" OFF)
option(ML_DOCUMENT_INTERNALS "Include internals in documentation" OFF)

if (ML_BUILD_DOCS)
    set(DOXYGEN_SKIP_DOT TRUE)
    find_package(Doxygen)
endif()

#--------------------------------------------------------------------
# Compiler flags
#--------------------------------------------------------------------

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3")

#--------------------------------------------------------------------
# Choose library output name
#--------------------------------------------------------------------

set(madronalib_NAME madrona)

#--------------------------------------------------------------------
# Enforce out of source build
#--------------------------------------------------------------------

if(CMAKE_BINARY_DIR EQUAL CMAKE_SOURCE_DIR)
  message(FATAL_ERROR "Madronalib requires an out of source build")
endif()

#--------------------------------------------------------------------
# Add include directories
#--------------------------------------------------------------------

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/aes256)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/cJSON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/ffft)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/fix_str)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/juce)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/oscpack)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/portaudio)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/tinyxml) # to remove
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/utf)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source/core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source/DSP)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source/procs)

#--------------------------------------------------------------------
# Add subdirectories
#--------------------------------------------------------------------

add_subdirectory(source)
add_subdirectory(external/aes256)
add_subdirectory(external/cJSON)
add_subdirectory(external/oscpack)
add_subdirectory(external/portaudio)
add_subdirectory(external/utf)


if(BUILD_NEW_ONLY)
else()
	set(ML_JUCE_HEADER_PATH "/external/juce")
	add_subdirectory(external/juce)
	#target_link_libraries(madronalib external/juce)

endif()

if (ML_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (ML_BUILD_TESTS)
    add_subdirectory(tests)
endif()

#if (DOXYGEN_FOUND AND ML_BUILD_DOCS)
#    add_subdirectory(docs)
#endif()


