# madronalib/source/CMakeLists.txt

include_directories("${madronalib_SOURCE_DIR}/src"
                    "${madronalib_BINARY_DIR}/src"
                    ${madronalib_INCLUDE_DIRS})

#add_definitions(-D_MADRONALIB_USE_CONFIG_H)


set(NETWORKING_SOURCES
    networking/MLNetServiceHub.cpp
    networking/MLOSCListener.cpp
    networking/MLOSCReceiver.cpp
    networking/MLOSCSender.cpp
    networking/MLT3DHub.cpp
)



#--------------------------------------------------------------------
# collect sources and headers
#--------------------------------------------------------------------

file(GLOB APP_SOURCES "app/*.cpp")
file(GLOB APP_HEADERS "app/*.h")

# DSP code is headers-only
file(GLOB DSP_HEADERS "DSP/*.h")

file(GLOB MATRIX_SOURCES "matrix/*.cpp")
file(GLOB MATRIX_HEADERS "matrix/*.h")

file(GLOB NETWORKING_SOURCES "networking/*.cpp")
file(GLOB NETWORKING_HEADERS "networking/*.h")

file(GLOB PROC_SOURCES "procs/*.cpp")
file(GLOB PROC_HEADERS "procs/*.h")

set(AES_SOURCES "../external/aes256/aes256.cpp")
set(JSON_SOURCES "../external/cJSON/cJSON.c")

file(GLOB OSC_HEADERS_IP "../external/oscpack/ip/*.h")
file(GLOB OSC_HEADERS_OSC "../external/oscpack/osc/*.h")
file(GLOB OSC_HEADERS_ZEROCONF "../external/oscpack/zeroconf/*.h")

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


install(FILES ../include/madronalib.h DESTINATION include/madronalib)

install(FILES ../include/mldsp.h DESTINATION include/madronalib)

install(FILES ../external/cJSON/cJSON.h DESTINATION include/madronalib)

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

target_include_directories(madronalib PUBLIC app)
target_include_directories(madronalib PUBLIC DSP)
target_include_directories(madronalib PUBLIC matrix)
target_include_directories(madronalib PUBLIC networking)
target_include_directories(madronalib PUBLIC procs)

target_include_directories(madronalib PUBLIC "../external/aes256")
target_include_directories(madronalib PUBLIC "../external/cJSON")
target_include_directories(madronalib PUBLIC "../external/ffft")
target_include_directories(madronalib PUBLIC "../external/oscpack")
target_include_directories(madronalib PUBLIC "../external/utf")

target_include_directories(madronalib PUBLIC "../include")

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

# Juce Libraries

if(BUILD_NEW_ONLY)
else(BUILD_NEW_ONLY)
  target_link_libraries(madronalib juce_audio_basics)
  target_link_libraries(madronalib juce_audio_devices)
  target_link_libraries(madronalib juce_core)
  target_link_libraries(madronalib juce_graphics)
  target_link_libraries(madronalib juce_gui_basics)
  target_link_libraries(madronalib juce_gui_extra)
  target_link_libraries(madronalib juce_opengl)
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
  #target_link_libraries(madronalib ${DNSSD_LIBRARIES})
endif()

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
