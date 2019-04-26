# madronalib/CMakeLists.txt
# root CMake file for madronalib project.

# Windows cmd line: cmake .. -G "Visual Studio 14 2015 Win64"


#--------------------------------------------------------------------
# set min version and deployment target -- before project
#--------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.10" CACHE STRING "Minimum OS X deployment version")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

#--------------------------------------------------------------------
# project and version
#--------------------------------------------------------------------

project(madronalib CXX)

set(madronalib_VERSION_MAJOR "0")
set(madronalib_VERSION_MINOR "1")
set(madronalib_VERSION_PATCH "0")
set(madronalib_VERSION_EXTRA "")
set(madronalib_VERSION "${madronalib_VERSION_MAJOR}.${madronalib_VERSION_MINOR}")
set(madronalib_VERSION_FULL "${madronalib_VERSION}.${madronalib_VERSION_PATCH}${madronalib_VERSION_EXTRA}")

# set_property(GLOBAL PROPERTY USE_FOLDERS ON)

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



# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/aes256)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/cJSON)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/ffft)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/juce)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/utf)

# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source/app)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source/DSP)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source/matrix)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source/networking)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source/procs)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if(APPLE)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/oscpack)
else()
endif()

#--------------------------------------------------------------------
# Add subdirectories
#--------------------------------------------------------------------

# add_subdirectory(source)


if(APPLE)
	add_subdirectory(external/oscpack)
else()
  #TODO OSC for Windows / Linux
endif()

if (ML_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if (ML_BUILD_TESTS)
   add_subdirectory(tests)
endif()

# madronalib/source/CMakeLists.txt

#include_directories("${madronalib_SOURCE_DIR}/src"
 #                   "${madronalib_BINARY_DIR}/src"
  #                  ${madronalib_INCLUDE_DIRS})

#	add_definitions(-D_MADRONALIB_USE_CONFIG_H)


#--------------------------------------------------------------------
# collect sources and headers
#--------------------------------------------------------------------

file(GLOB APP_SOURCES "source/app/*.cpp")
file(GLOB APP_HEADERS "source/app/*.h")

# DSP code is headers-only
file(GLOB DSP_HEADERS "source/DSP/*.h")

file(GLOB MATRIX_SOURCES "source/matrix/*.cpp")
file(GLOB MATRIX_HEADERS "source/matrix/*.h")

file(GLOB NETWORKING_SOURCES "source/networking/*.cpp")
file(GLOB NETWORKING_HEADERS "source/networking/*.h")

file(GLOB PROC_SOURCES "source/procs/*.cpp")
file(GLOB PROC_HEADERS "source/procs/*.h")

set(AES_SOURCES "external/aes256/aes256.cpp")
set(JSON_SOURCES "external/cJSON/cJSON.c")

file(GLOB OSC_HEADERS_IP "external/oscpack/ip/*.h")
file(GLOB OSC_HEADERS_OSC "external/oscpack/osc/*.h")
file(GLOB OSC_HEADERS_ZEROCONF "external/oscpack/zeroconf/*.h")

set(OSC_HEADERS
    ${OSC_HEADERS_IP}
    ${OSC_HEADERS_OSC}
    ${OSC_HEADERS_ZEROCONF}
)

set(madronalib_SOURCES
    ${APP_SOURCES}
    ${MATRIX_SOURCES}
    ${PROC_SOURCES}
    ${AES_SOURCES}
    ${JSON_SOURCES}
    )

if(APPLE)
  list(APPEND madronalib_SOURCES ${NETWORKING_SOURCES} )
else()
  #TODO OSC for windows
endif()



# send binary output to the current build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#--------------------------------------------------------------------
# set source groups for source above
#--------------------------------------------------------------------

source_group(dsp REGULAR_EXPRESSION "DSP/.*")
source_group(app REGULAR_EXPRESSION "app/.*")
source_group(matrix REGULAR_EXPRESSION "matrix/.*")
source_group(networking REGULAR_EXPRESSION "networking/.*")
source_group(procs REGULAR_EXPRESSION "procs/.*")

#--------------------------------------------------------------------
# create and install library
#--------------------------------------------------------------------


add_library(madronalib STATIC ${madronalib_SOURCES})
set_target_properties(madronalib PROPERTIES
                      OUTPUT_NAME "${madronalib_NAME}"
                      VERSION ${madronalib_VERSION}
                      SOVERSION ${madronalib_VERSION_MAJOR}
                      POSITION_INDEPENDENT_CODE ON
                      FOLDER "madronalib")

include(GNUInstallDirs)

install(
    TARGETS madronalib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

#--------------------------------------------------------------------
# install headers
#--------------------------------------------------------------------


install(FILES include/madronalib.h DESTINATION include/madronalib)

install(FILES include/mldsp.h DESTINATION include/madronalib)

install(FILES external/cJSON/cJSON.h DESTINATION include/madronalib)

install(FILES 
    ${APP_HEADERS}     
    ${DSP_HEADERS}
    ${MATRIX_HEADERS} 
    ${PROC_HEADERS} 
    ${OSC_HEADERS}
    DESTINATION include/madronalib
  )

if(APPLE)
    install(FILES ${NETWORKING_HEADERS} DESTINATION include/madronalib)
else()
    #TODO OSC For win / Linux
endif()


#--------------------------------------------------------------------
# Add include Directories
#--------------------------------------------------------------------

# target_include_directories(madronalib PUBLIC source/app)
# target_include_directories(madronalib PUBLIC source/DSP)
# target_include_directories(madronalib PUBLIC source/matrix)
# target_include_directories(madronalib PUBLIC source/networking)
# target_include_directories(madronalib PUBLIC source/procs)
# 
# target_include_directories(madronalib PUBLIC "external/aes256")
# target_include_directories(madronalib PUBLIC "external/cJSON")
# target_include_directories(madronalib PUBLIC "external/ffft")
# target_include_directories(madronalib PUBLIC "external/oscpack")
# target_include_directories(madronalib PUBLIC "external/utf")
# 
# target_include_directories(madronalib PUBLIC "include")

#--------------------------------------------------------------------
# Make docs (someday)
#--------------------------------------------------------------------


#if (DOXYGEN_FOUND AND ML_BUILD_DOCS)
#    add_subdirectory(docs)
#endif()


#--------------------------------------------------------------------
# Add libraries
#--------------------------------------------------------------------


# MLTEST - switching to source
# target_link_libraries(madronalib aes256)
# target_link_libraries(madronalib cjson)

# note: ffft and utf are header-only



if(APPLE)
    target_link_libraries(madronalib oscpack)
else()
endif()


if(APPLE)
  target_link_libraries(madronalib "-framework Carbon")
  target_link_libraries(madronalib "-framework CoreServices")
  target_link_libraries(madronalib "-framework AudioToolbox")
  target_link_libraries(madronalib "-framework AudioUnit")
  target_link_libraries(madronalib "-framework CoreAudio")
  target_link_libraries(madronalib "-framework Foundation")
  target_link_libraries(madronalib "-framework OpenGL")
  target_link_libraries(madronalib "-framework GLUT")
else(APPLE)
  # target_link_libraries(madronalib ${DNSSD_LIBRARIES})
endif()


#--------------------------------------------------------------------
# unused
#--------------------------------------------------------------------

if (BUILD_SHARED_LIBS)
    if (WIN32)
        # The MADRONALIB DLL needs a special compile-time macro and import library name
        set_target_properties(madronalib PROPERTIES PREFIX "" IMPORT_PREFIX "")

        if (MINGW)
            set_target_properties(madronalib PROPERTIES IMPORT_SUFFIX "dll.a")
        else()
            set_target_properties(madronalib PROPERTIES IMPORT_SUFFIX "dll.lib")
        endif()
    elseif (APPLE)
        # Append -fno-common to the compile flags to work around a bug in
        # Apple's GCC
        get_target_property(madronalib_CFLAGS madronalib COMPILE_FLAGS)
        if (NOT madronalib_CFLAGS)
            set(madronalib_CFLAGS "")
        endif()
        set_target_properties(madronalib PROPERTIES
                              COMPILE_FLAGS "${madronalib_CFLAGS} -fno-common"
                              INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")
    endif()

    target_link_libraries(madronalib ${madronalib_LIBRARIES})
endif()



