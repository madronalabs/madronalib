
# madronalib/examples/CMakeLists.txt
# CMake file for madronalib examples.

link_libraries(madronalib)

if (BUILD_SHARED_LIBS)
    add_definitions(-DMADRONALIB_DLL)
    link_libraries("${OPENGL_gl_LIBRARY}" "${MATH_LIBRARY}")
else()
    link_libraries(${madronalib_LIBRARIES})
endif()

include_directories("${MADRONALIB_SOURCE_DIR}/include"
                    "${MADRONALIB_SOURCE_DIR}/deps")

if ("${OPENGL_INCLUDE_DIR}")
    include_directories("${OPENGL_INCLUDE_DIR}")
endif()

#--------------------------------------------------------------------
# Compiler flags
#--------------------------------------------------------------------

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# find portaudio library by executing cmake/FindPortaudio.cmake
# and add portaudio example if found

find_package(portaudio)

if (PORTAUDIO_FOUND)
    add_executable(portaudio_example portaudioExample.cpp)
    target_link_libraries(portaudio_example ${PORTAUDIO_LIBRARY})
endif (PORTAUDIO_FOUND)

#--------------------------------------------------------------------
# Add the other examples.
#--------------------------------------------------------------------

add_executable(dsp_ops_example dspOpsExample.cpp)
add_executable(symbols_example symbolsExample.cpp)
add_executable(symbols_example portaudioExample.cpp)


