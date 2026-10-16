# madronalib/CMakeLists.txt
# root CMake file for madronalib project.

# Windows cmd line: cmake .. -G "Visual Studio 14 2015 Win64"


#--------------------------------------------------------------------
# set min version and deployment target -- before project
#--------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

IF(APPLE)
  SET(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)
  
  # Also set for the C and C++ compilers specifically
  set(CMAKE_C_OSX_ARCHITECTURES "x86_64;arm64")
  set(CMAKE_CXX_OSX_ARCHITECTURES "x86_64;arm64")
  
  SET(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
ENDIF(APPLE)


#--------------------------------------------------------------------
# project and version
#--------------------------------------------------------------------

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

project(madronalib)

#--------------------------------------------------------------------
# FetchContent for cJSON (source only - we compile cJSON.c directly into madronalib)
#--------------------------------------------------------------------

include(FetchContent)

FetchContent_Declare(
  cjson
  GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
  GIT_TAG v1.7.18
  SOURCE_SUBDIR this_does_not_exist  # Prevents add_subdirectory, we just want the source
)

FetchContent_MakeAvailable(cjson)

set(madronalib_VERSION_MAJOR "0")
set(madronalib_VERSION_MINOR "1")
set(madronalib_VERSION_PATCH "0")
set(madronalib_VERSION_EXTRA "")
set(madronalib_VERSION "${madronalib_VERSION_MAJOR}.${madronalib_VERSION_MINOR}")
set(madronalib_VERSION_FULL "${madronalib_VERSION}.${madronalib_VERSION_PATCH}${madronalib_VERSION_EXTRA}")

option(BUILD_EXAMPLES "Build the examples" ON)
option(BUILD_TESTS "Build the tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ML_BUILD_DOCS "Build the documentation" OFF)
option(ML_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
option(ML_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# The deserialization tests assert on behaviour that a plain build cannot see:
# a heap over-read returns plausible bytes and carries on, so REQUIRE passes
# while the bug is still there. Configure a second build directory with
# -DML_SANITIZE=ON to make those failures visible.
if(ML_SANITIZE)
    if(MSVC)
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

if (ML_BUILD_DOCS)
    set(DOXYGEN_SKIP_DOT TRUE)
    find_package(Doxygen)
endif()

#--------------------------------------------------------------------
# Compiler flags
#--------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#--------------------------------------------------------------------
# Platform-specific compiler flags
#--------------------------------------------------------------------

if(APPLE)
    # Debug info in Release builds
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
    
elseif(WIN32)
    # Suppress unknown pragma warnings and disable aligned new
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4068 /Zc:alignedNew-")

    # Compile a project's translation units in parallel. MSBuild's own -m only
    # parallelizes across projects, and madronalib is a single big one.
    # Appended here rather than passed as -DCMAKE_CXX_FLAGS, which would replace
    # CMake's default /DWIN32 /D_WINDOWS /EHsc instead of adding to it.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    
    # Debug info in Release builds (for profiling/debugging)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    
    # Release build optimizations for DSP performance
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")      # Optimize for speed
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /fp:fast") # Fast floating-point
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Oi")      # Intrinsic functions
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ot")      # Favor fast code
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")      # Whole program optimization
    
    # Link-time code generation (companion to /GL)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")

    # Symbols for the tests and example executables. /DEBUG turns /OPT:REF and
    # /OPT:ICF off by default, so name them explicitly; /DEBUG:FULL rather than
    # /DEBUG:FASTLINK so the PDBs stay self-contained. The madronalib target is a
    # static library archived by lib.exe, which takes none of these.
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG:FULL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF")
    
else()
    # GCC/Clang on Linux
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")

    # Enable SSE4.1 on x86_64 so SSE4 intrinsics in MLDSPMathSSE.h compile.
    # GCC's default x86_64 baseline is SSE2; _mm_mullo_epi32 needs SSE4.1.
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")
    endif()
endif()

#--------------------------------------------------------------------
# MSVC runtime library configuration
#--------------------------------------------------------------------

if(MSVC)
    # Use static runtime library (/MT)
    cmake_policy(SET CMP0091 NEW)
    add_compile_options(
        $<$<CONFIG:>:/MT>
        $<$<CONFIG:Debug>:/MTd>
        $<$<CONFIG:Release>:/MT>
    )
endif()

#--------------------------------------------------------------------
# Choose library output name
#--------------------------------------------------------------------

# creates the library madronalib-debug in debug configuration
set(madronalib_NAME madrona$<$<CONFIG:Debug>:-debug>)

#--------------------------------------------------------------------
# Enforce out of source build
#--------------------------------------------------------------------

if(CMAKE_BINARY_DIR EQUAL CMAKE_SOURCE_DIR)
  message(FATAL_ERROR "Madronalib requires an out of source build.")
endif()

# Include directories are set as target_include_directories after
# add_library below, so they propagate to add_subdirectory consumers.

#--------------------------------------------------------------------
# collect sources and headers
#--------------------------------------------------------------------

file(GLOB APP_SOURCES "source/app/*.cpp")
file(GLOB APP_HEADERS "source/app/*.h")

# DSP code is headers-only, except the Scala parser behind ml::Scale
file(GLOB DSP_SOURCES "source/DSP/*.cpp")
file(GLOB DSP_HEADERS "source/DSP/*.h")

# ODDSound MTS-ESP client (ISC). Only MLMTSClient.cpp includes it.
file(GLOB MTS_SOURCES "external/mts-esp/*.cpp")

file(GLOB MATRIX_SOURCES "source/matrix/*.cpp")
file(GLOB MATRIX_HEADERS "source/matrix/*.h")

file(GLOB AES_SOURCES "external/aes256/*.cpp")

# cJSON source (fetched via FetchContent, compiled directly into madronalib)
set(CJSON_SOURCES ${cjson_SOURCE_DIR}/cJSON.c)

file(GLOB OSC_HEADERS_IP "external/oscpack/ip/*.h")
file(GLOB OSC_HEADERS_OSC "external/oscpack/osc/*.h")
file(GLOB OSC_HEADERS_ZEROCONF "external/oscpack/zeroconf/*.h")

file(GLOB OSC_SOURCES_IP "external/oscpack/ip/*.cpp")
file(GLOB OSC_SOURCES_OSC "external/oscpack/osc/*.cpp")
# Zeroconf is POSIX-only (uses sys/select.h, DNS-SD)
if(NOT WIN32)
    file(GLOB OSC_SOURCES_ZEROCONF "external/oscpack/zeroconf/*.cpp")
endif()

file(GLOB RTAUDIO_SOURCES "external/rtaudio/*.cpp")
file(GLOB RTAUDIO_HEADERS "external/rtaudio/*.h")

file(GLOB RTMIDI_SOURCES "external/rtmidi/*.cpp")
file(GLOB RTMIDI_HEADERS "external/rtmidi/*.h")

file(GLOB EXTERNAL_HEADERS "include/*.h")


# Platform-specific OSC socket implementations
if(APPLE OR UNIX)
    file(GLOB OSC_SOURCES_NATIVE "external/oscpack/ip/posix/*.cpp")
elseif(WIN32)
    file(GLOB OSC_SOURCES_NATIVE "external/oscpack/ip/win32/*.cpp")
endif()

set(OSC_HEADERS
    ${OSC_HEADERS_IP}
    ${OSC_HEADERS_OSC}
    ${OSC_HEADERS_ZEROCONF}
)

# set the sources - we include the headers only to make them
# navigable within IDEs

set(madronalib_SOURCES
    ${EXTERNAL_HEADERS}
    ${APP_SOURCES}
    ${APP_HEADERS}
    ${RTAUDIO_SOURCES}
    ${RTAUDIO_HEADERS}
    ${RTMIDI_SOURCES}
    ${RTMIDI_HEADERS}
    ${DSP_SOURCES}
    ${MTS_SOURCES}
    ${DSP_HEADERS}
    ${MATRIX_SOURCES}
    ${MATRIX_HEADERS}
    ${AES_SOURCES}
    ${CJSON_SOURCES}
)

    message("RTMIDI_SOURCES: " ${RTMIDI_SOURCES} )

# OSC support - cross-platform
list(APPEND madronalib_SOURCES ${OSC_SOURCES_IP} )
list(APPEND madronalib_SOURCES ${OSC_SOURCES_OSC} )
if(NOT WIN32)
    list(APPEND madronalib_SOURCES ${OSC_SOURCES_ZEROCONF} )
endif()
list(APPEND madronalib_SOURCES ${OSC_SOURCES_NATIVE} )

# send binary output to the current build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#--------------------------------------------------------------------
# set source groups for some source above
#--------------------------------------------------------------------

source_group(dsp REGULAR_EXPRESSION "DSP/.*")
source_group(app REGULAR_EXPRESSION "app/.*")
source_group(matrix REGULAR_EXPRESSION "matrix/.*")
source_group(networking REGULAR_EXPRESSION "networking/.*")
source_group(external REGULAR_EXPRESSION "external/.*")

#--------------------------------------------------------------------
# create and install library
#--------------------------------------------------------------------

set(target madronalib)

add_library(${target} STATIC ${madronalib_SOURCES})

# Force Xcode to respect the architectures
set_target_properties(${target} PROPERTIES
    XCODE_ATTRIBUTE_ARCHS "x86_64 arm64"
    XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "NO"
    XCODE_ATTRIBUTE_VALID_ARCHS "x86_64 arm64"
    OSX_ARCHITECTURES "x86_64;arm64" 
)

set_target_properties(${target} PROPERTIES
                      OUTPUT_NAME "${madronalib_NAME}"
                      VERSION ${madronalib_VERSION}
                      SOVERSION ${madronalib_VERSION_MAJOR}
                      POSITION_INDEPENDENT_CODE ON
                      FOLDER "madronalib"
                      # Hide all symbols by default so multiple plugins using madronalib
                      # in the same host process don't collide via the dynamic symbol table.
                      CXX_VISIBILITY_PRESET hidden
                      VISIBILITY_INLINES_HIDDEN ON)

# Headers install destination (also used by install() commands below)
if(APPLE)
    set(HEADERS_INCLUDE_DIR "include/madronalib")
elseif(WIN32)
    set(HEADERS_INCLUDE_DIR "include")
else()
    set(HEADERS_INCLUDE_DIR "include/madronalib")  # Linux
endif()

# Public include dirs propagate to consumers via target_link_libraries.
# BUILD_INTERFACE paths are used by add_subdirectory consumers;
# INSTALL_INTERFACE paths are used after cmake --install.
target_include_directories(${target} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/DSP>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/app>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/matrix>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/networking>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/oscpack>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/oscpack/ip>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/oscpack/osc>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/oscpack/zeroconf>
    $<BUILD_INTERFACE:${cjson_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${HEADERS_INCLUDE_DIR}>
)
# aes256, utf, tuning-library and mts-esp use flat includes (e.g. #include "aes256.h")
target_include_directories(${target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/aes256
    ${CMAKE_CURRENT_SOURCE_DIR}/external/utf
    ${CMAKE_CURRENT_SOURCE_DIR}/external/tuning-library
    ${CMAKE_CURRENT_SOURCE_DIR}/external/mts-esp
)
# libMTSClient.cpp dlopens libMTS at static-init time; consumers need libdl on Linux
target_link_libraries(${target} PUBLIC ${CMAKE_DL_LIBS})

# cJSON is now compiled directly into madronalib (no separate target)
# cJSON.h is installed alongside madronalib headers for consumers

# choose driver for Rtaudio
if(WIN32)
    target_compile_definitions(${target} PRIVATE __WINDOWS_WASAPI__)
elseif(APPLE)
    target_compile_definitions(${target} PRIVATE __MACOSX_CORE__)
elseif(LINUX_JACK)
    target_compile_definitions(${target} PRIVATE __UNIX_JACK__)
    target_link_libraries(${target} PRIVATE jack pthread)
elseif(LINUX_PULSE)
    target_compile_definitions(${target} PRIVATE __LINUX_PULSE__)
    target_link_libraries(${target} PRIVATE pulse-simple pulse pthread)
elseif(LINUX_ASOUND)
    target_compile_definitions(${target} PRIVATE __LINUX_ALSA__)
    target_link_libraries(${target} PRIVATE asound pthread)
else()
    message(FATAL, "On Linux, one of LINUX_JACK, LINUX_PULSE, or LINUX_ASOUND must be set.")
endif()

if(APPLE)
    set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
endif()

include(GNUInstallDirs)

if(WIN32)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
      set(CMAKE_INSTALL_PREFIX "C:/Program Files/madronalib" CACHE PATH "..." FORCE)
    endif()
    # Let GNUInstallDirs set CMAKE_INSTALL_LIBDIR relative to CMAKE_INSTALL_PREFIX
endif()

install(
    TARGETS ${target}
    EXPORT madronalib-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT library
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT library
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT library
)

# Install CMake package config files
include(CMakePackageConfigHelpers)

install(EXPORT madronalib-targets
    FILE madronalibTargets.cmake
    NAMESPACE madronalib::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/madronalib
    COMPONENT library
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/madronalibConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/madronalibConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/madronalib
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/madronalibConfigVersion.cmake
    VERSION ${madronalib_VERSION_FULL}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/madronalibConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/madronalibConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/madronalib
    COMPONENT library
)

message("madronalib destination: " ${CMAKE_INSTALL_LIBDIR} )

#--------------------------------------------------------------------
# install headers
#--------------------------------------------------------------------

# HEADERS_INCLUDE_DIR is defined earlier (used here and by target_include_directories)

install(FILES include/madronalib.h DESTINATION ${HEADERS_INCLUDE_DIR} COMPONENT library)
install(FILES include/mldsp.h DESTINATION ${HEADERS_INCLUDE_DIR} COMPONENT library)

# TODO: don't install this, wrap
# Install cJSON header (fetched via FetchContent, consumers get it from here)
# install(FILES ${cjson_SOURCE_DIR}/cJSON.h DESTINATION ${HEADERS_INCLUDE_DIR})

install(FILES
    ${APP_HEADERS}
    ${DSP_HEADERS}
    ${MATRIX_HEADERS}
    ${OSC_HEADERS}
    DESTINATION
    # PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    ${HEADERS_INCLUDE_DIR}
    COMPONENT library
  )

# OSC headers are installed as part of APP_HEADERS (now in source/app/)

#--------------------------------------------------------------------
# Add platform libraries
#--------------------------------------------------------------------

# note: ffft and utf are header-only
# aes256 and cJSON added as source (cJSON fetched via FetchContent)

if(APPLE)
  target_link_libraries(madronalib PRIVATE "-framework Carbon")
  target_link_libraries(madronalib PRIVATE "-framework CoreServices")
  target_link_libraries(madronalib PRIVATE "-framework CoreMIDI")
  target_link_libraries(madronalib PRIVATE "-framework AudioToolbox")
  target_link_libraries(madronalib PRIVATE "-framework AudioUnit")
  target_link_libraries(madronalib PRIVATE "-framework CoreAudio")
  target_link_libraries(madronalib PRIVATE "-framework Foundation")
  target_link_libraries(madronalib PRIVATE "-framework OpenGL")
  target_link_libraries(madronalib PRIVATE "-framework GLUT")
elseif(WIN32)
  # winmm for multimedia timer, ws2_32 for Winsock2 (OSC networking)
  target_link_libraries(madronalib PRIVATE winmm.lib ws2_32.lib)
endif()
# Linux linking handled earlier via LINUX_ASOUND/LINUX_JACK/LINUX_PULSE

#--------------------------------------------------------------------
# function to build RtAudio / RtMidi example
#--------------------------------------------------------------------

set(ML_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

function(make_example EXAMPLE_APP_NAME EXAMPLE_SOURCE_FILE)

    set(
        EXAMPLE_SOURCES
        ${ML_ROOT}/examples/audio-and-midi/${EXAMPLE_SOURCE_FILE}
    )

    add_executable(${EXAMPLE_APP_NAME} ${EXAMPLE_SOURCES})

    target_include_directories(${EXAMPLE_APP_NAME} PRIVATE
        ${ML_ROOT}/external
        )

    target_link_libraries(${EXAMPLE_APP_NAME} PRIVATE madronalib)

    # Link platform audio/midi libraries for examples (needed since madronalib is static)
    if(LINUX_JACK)
        target_link_libraries(${EXAMPLE_APP_NAME} PRIVATE jack pthread)
    elseif(LINUX_PULSE)
        target_link_libraries(${EXAMPLE_APP_NAME} PRIVATE pulse-simple pulse pthread)
    elseif(LINUX_ASOUND)
        target_link_libraries(${EXAMPLE_APP_NAME} PRIVATE asound pthread)
    endif()

    set_target_properties(${EXAMPLE_APP_NAME}
        PROPERTIES
        MACOSX_BUNDLE TRUE

        # not working, see below for workaround adding to plist
        # MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in

        MACOSX_BUNDLE_BUNDLE_NAME ${EXAMPLE_APP_NAME}
        MACOSX_BUNDLE_BUNDLE_VERSION "0.9.0"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # get directory of .app package Contents
    set(PACKAGE_DIR
        "${CMAKE_BINARY_DIR}/bin/${EXAMPLE_APP_NAME}.app/Contents"
        )

    # add missing keys to plist for Mac OS
    if(APPLE)
        set(ADD_PLIST_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/scripts/add-plist-value.sh")
        set(RATIONALE "This app requires microphone access.")
        add_custom_command(
            TARGET ${EXAMPLE_APP_NAME} POST_BUILD
            COMMAND ${ADD_PLIST_SCRIPT} NSMicrophoneUsageDescription "This app requires microphone access." "${PACKAGE_DIR}/Info.plist"
            VERBATIM
        )
    endif()
endfunction()

#--------------------------------------------------------------------
# function to build simple console example
#--------------------------------------------------------------------

function(make_console_example EXAMPLE_APP_NAME EXAMPLE_SOURCE_FILE EXAMPLE_SUBDIR)

    set(
        EXAMPLE_SOURCES
        ${ML_ROOT}/examples/${EXAMPLE_SUBDIR}/${EXAMPLE_SOURCE_FILE}
    )

    add_executable(${EXAMPLE_APP_NAME} ${EXAMPLE_SOURCES})

    target_include_directories(${EXAMPLE_APP_NAME} PRIVATE
        ${ML_ROOT}/external
        )

    target_link_libraries(${EXAMPLE_APP_NAME} PRIVATE madronalib)

    set_target_properties(${EXAMPLE_APP_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endfunction()

#--------------------------------------------------------------------
# build all examples
#--------------------------------------------------------------------

if(BUILD_EXAMPLES)
  make_example(ReverbExample reverb.cpp)
  make_example(FDTDExample fdtd.cpp)
  make_example(ParamsExample params.cpp)
  make_example(MIDIExample midi.cpp)
  make_example(ControllersToAudioExample controllers-to-audio.cpp)
  make_example(SineExample sine.cpp)
  make_example(Study1Example study1.cpp)

  # OSC examples (simple console apps)
  make_console_example(osc-listener osc-listener.cpp osc)
  make_console_example(osc-sender osc-sender.cpp osc)
endif()

#--------------------------------------------------------------------
# build tests
#--------------------------------------------------------------------

if(BUILD_TESTS)
    set(target tests)
    file(GLOB TEST_SOURCES "Tests/*.*")

    add_executable(tests ${TEST_SOURCES})
    add_dependencies(tests madronalib)

    target_link_libraries(tests madronalib)

endif()

#--------------------------------------------------------------------
# Including custom cmake rules
#--------------------------------------------------------------------

set(madronalib_SOURCES_internal
    ${APP_SOURCES}
    ${APP_HEADERS}
    ${DSP_HEADERS}
    ${MATRIX_SOURCES}
    ${MATRIX_HEADERS}
    )

# Adding clang-format target if executable is found
find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
  add_custom_target(
    clang-format
    COMMAND /opt/homebrew/bin/clang-format
    -i
    --verbose
    -style=file
    ${madronalib_SOURCES_internal}
    )
endif()
