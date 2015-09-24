
# madronalib/tests/CMakeLists.txt
# CMake file for madronalib project tests.

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

#--------------------------------------------------------------------
# Add all the tests.
#--------------------------------------------------------------------

add_executable(tests catch.hpp tests.cpp symbolTest.cpp signalTest.cpp resourceMapTest.cpp)

