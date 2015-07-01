# madronalib/CMakeLists.txt
# root CMake file for madronalib project.

project(madronalib CXX)

cmake_minimum_required(VERSION 2.8.12)

if (NOT CMAKE_VERSION VERSION_LESS "3.0")
    # Until all major package systems have moved to CMake 3,
    # we stick with the older INSTALL_NAME_DIR mechanism
    cmake_policy(SET CMP0042 OLD)
endif()

set(madronalib_VERSION_MAJOR "0")
set(madronalib_VERSION_MINOR "1")
set(madronalib_VERSION_PATCH "0")
set(madronalib_VERSION_EXTRA "")
set(madronalib_VERSION "${madronalib_VERSION_MAJOR}.${madronalib_VERSION_MINOR}")
set(madronalib_VERSION_FULL "${madronalib_VERSION}.${madronalib_VERSION_PATCH}${madronalib_VERSION_EXTRA}")
set(LIB_SUFFIX "" CACHE STRING "Takes an empty string or 64. Directory where lib will be installed: lib or lib64")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ML_BUILD_TESTS "Build the ML test programs" ON)
option(ML_BUILD_DOCS "Build the ML documentation" ON)
option(ML_INSTALL "Generate installation target" ON)
option(ML_DOCUMENT_INTERNALS "Include internals in documentation" OFF)

if (ML_BUILD_DOCS)
    set(DOXYGEN_SKIP_DOT TRUE)
    find_package(Doxygen)
endif()

#--------------------------------------------------------------------
# Compiler flags
#--------------------------------------------------------------------

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


#--------------------------------------------------------------------
# Choose library output name
#--------------------------------------------------------------------

set(madronalib_NAME madrona)

#--------------------------------------------------------------------
# Enforce out of source build
#--------------------------------------------------------------------


set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
include(MacroOutOfSourceBuild)
macro_ensure_out_of_source_build("${PROJECT_NAME} requires an out of source build.")

#--------------------------------------------------------------------
# Add subdirectories
#--------------------------------------------------------------------
    
add_subdirectory(source)


if (ML_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if (DOXYGEN_FOUND AND ML_BUILD_DOCS)
    add_subdirectory(docs)
endif()


