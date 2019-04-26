
# madronalib/tests/CMakeLists.txt
# CMake file for madronalib project tests.

#--------------------------------------------------------------------
# set min version and deployment target -- before project
#--------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.10" CACHE STRING "Minimum OS X deployment version")


#--------------------------------------------------------------------
# Compiler flags
#--------------------------------------------------------------------

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


#--------------------------------------------------------------------
# Add all the tests.
#--------------------------------------------------------------------

add_executable(tests 
	catch.hpp 
	dspOpsTest.cpp
	OSCTest.cpp
	procsTest.cpp
	queueTest.cpp
	treeTest.cpp 
	dspBufferTest.cpp 
	symbolTest.cpp
	timerTest.cpp
	tests.cpp
	tests.h 
	)


#--------------------------------------------------------------------
# Add include Directories
#--------------------------------------------------------------------

target_include_directories(tests PUBLIC ../source/app)
target_include_directories(tests PUBLIC ../source/DSP)
target_include_directories(tests PUBLIC ../source/matrix)
target_include_directories(tests PUBLIC ../source/networking)
target_include_directories(tests PUBLIC ../source/procs)

target_include_directories(tests PUBLIC "../external/aes256")
target_include_directories(tests PUBLIC "../external/cJSON")
target_include_directories(tests PUBLIC "../external/ffft")
target_include_directories(tests PUBLIC "../external/oscpack")
target_include_directories(tests PUBLIC "../external/utf")

target_include_directories(tests PUBLIC "../include")

#--------------------------------------------------------------------
# Link with libraries
#--------------------------------------------------------------------

# madronalib
find_library(MADRONA_LIB madrona)
target_link_libraries(tests "${MADRONA_LIB}")

# TEMP
message("tests->madronalib:" ${MADRONA_LIB} )

link_libraries(madronalib)


